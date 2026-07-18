import argparse
import asyncio
from datetime import datetime
import json
import subprocess
import sys
import threading
from pathlib import Path
from typing import Callable, Optional

import numpy as np
from bleak.exc import BleakError
from bleak import BleakClient, BleakScanner

sd = None
AudioUtilities = None
IAudioEndpointVolume = None
CLSCTX_ALL = None

if sys.platform == "win32" and sys.version_info < (3, 14):
    try:
        asyncio.set_event_loop_policy(asyncio.WindowsProactorEventLoopPolicy())
    except AttributeError:
        try:
            asyncio.set_event_loop_policy(asyncio.WindowsSelectorEventLoopPolicy())
        except AttributeError:
            pass


def _get_audio_utilities():
    global AudioUtilities
    if AudioUtilities is None:
        try:
            from pycaw.pycaw import AudioUtilities as _AudioUtilities

            AudioUtilities = _AudioUtilities
        except Exception:
            try:
                import traceback

                print("_get_audio_utilities: failed to import pycaw:")
                traceback.print_exc()
            except Exception:
                pass
            AudioUtilities = None
    return AudioUtilities


def _get_windows_endpoint_volume():
    global AudioUtilities

    if sys.platform != "win32":
        return None
    try:
        audio_utils = _get_audio_utilities()
        if audio_utils is None:
            return None
        
        speakers = audio_utils.GetSpeakers()
        return speakers.EndpointVolume
    except Exception:
        try:
            import traceback

            print("_get_windows_endpoint_volume: exception while acquiring endpoint:")
            traceback.print_exc()
        except Exception:
            pass
        return None


def _get_sounddevice():
    global sd
    if sd is None:
        try:
            import sounddevice as sounddevice_module

            sd = sounddevice_module
        except Exception:
            sd = None
    return sd

SERVICE_UUID = "12345678-1234-1234-1234-1234567890ab"
CMD_UUID = "12345678-1234-1234-1234-1234567890ac"
STATE_UUID = "12345678-1234-1234-1234-1234567890ad"

DEFAULT_BUTTON_MAP = {
    1: "play_pause",
    2: "prev",
    3: "next",
    4: "mode",
    5: "mute",
}

DEFAULT_DISPLAY_MODE = "fft"
TIME_SYNC_INTERVAL_SECONDS = 30
FFT_BARS = 16
MIN_FREQ = 50
MAX_FREQ = 12000
VISUAL_ALPHABET = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"


class AudioController:
    def __init__(self):
        self.volume = 0.5
        self.is_playing = True

    def update_volume(self, value: float) -> None:
        self.volume = max(0.0, min(1.0, value))
        endpoint_volume = _get_windows_endpoint_volume()
        print(f"update_volume called: value={self.volume}, endpoint_volume={'present' if endpoint_volume is not None else 'none'}")
        if endpoint_volume is not None:
            try:
                endpoint_volume.SetMasterVolumeLevelScalar(self.volume, None)
            except Exception as e:
                print(f"Failed to set Windows volume via endpoint: {e}")
            return

        # Fallback: try winmm waveOutSetVolume (coarse, system-wide)
        if sys.platform == "win32":
            try:
                import ctypes

                winmm = ctypes.windll.winmm
                vol = int(self.volume * 0xFFFF) & 0xFFFF
                left_right = (vol << 16) | vol
                winmm.waveOutSetVolume(0xFFFFFFFF, left_right)
                print(f"Fallback waveOutSetVolume applied: {vol}")
                return
            except Exception as e:
                print(f"Failed to set volume fallback: {e}")

    def set_play_state(self, playing: bool) -> None:
        self.is_playing = playing

    def send_media_command(self, command: str) -> None:
        if self._send_native_media_command(command):
            return
        try:
            import pyautogui  # type: ignore

            if command == "prev":
                pyautogui.press("prevtrack")
            elif command == "next":
                pyautogui.press("nexttrack")
            elif command == "play_pause":
                pyautogui.press("playpause")
            elif command == "mute":
                pyautogui.press("volumemute")
            elif command == "mode":
                print("Mode switch requested")
            else:
                print(f"Media command: {command}")
        except Exception:
            print(f"Media command: {command}")

    def _send_native_media_command(self, command: str) -> bool:
        if sys.platform == "win32":
            return self._send_windows_media_command(command)
        if sys.platform == "darwin":
            return self._send_macos_media_command(command)
        return False

    def _send_windows_media_command(self, command: str) -> bool:
        key_codes = {
            "prev": 0xB1,
            "next": 0xB0,
            "play_pause": 0xB3,
            "mute": 0xAD,
        }
        key_code = key_codes.get(command)
        if key_code is None:
            return False
        try:
            import ctypes

            keyeventf_keyup = 0x0002
            ctypes.windll.user32.keybd_event(key_code, 0, 0, 0)
            ctypes.windll.user32.keybd_event(key_code, 0, keyeventf_keyup, 0)
            return True
        except Exception:
            return False

    def _send_macos_media_command(self, command: str) -> bool:
        scripts = {
            "prev": 'tell application "Music" to previous track',
            "next": 'tell application "Music" to next track',
            "play_pause": 'tell application "System Events" to key code 16 using {command down}',
            "mute": "set volume with output muted",
        }
        script = scripts.get(command)
        if script is None:
            return False
        try:
            subprocess.run(["osascript", "-e", script], check=False, capture_output=True)
            return True
        except Exception:
            return False


class BLEMusicController:
    def __init__(
        self,
        config_path: Optional[Path] = None,
        disable_audio: bool = False,
        clock_enabled: bool = True,
    ):
        self.client: Optional[BleakClient] = None
        self.device = None
        self.audio = AudioController()
        self.loop = None
        self.running = True
        self.connected = False
        self.sample_rate = 44100
        self.chunk_size = 1024
        self._fft_buffer = np.zeros(self.chunk_size, dtype=np.float32)
        self._visual_smooth = np.zeros(FFT_BARS, dtype=np.float32)
        self.disable_audio = disable_audio
        self.clock_enabled = clock_enabled
        self.config_path = config_path or Path(__file__).with_name("controller_config.json")
        self.button_map = DEFAULT_BUTTON_MAP.copy()
        self.display_mode = DEFAULT_DISPLAY_MODE
        self.on_log: Optional[Callable[[str], None]] = None
        self.on_state: Optional[Callable[[dict], None]] = None
        self.mode = 'A'
        self.load_config()

    def log(self, message: str) -> None:
        print(message)
        if self.on_log:
            self.on_log(message)

    def emit_state(self) -> None:
        if self.on_state:
            self.on_state(
                {
                    "connected": self.connected,
                    "volume": int(self.audio.volume * 100),
                    "playing": self.audio.is_playing,
                    "display_mode": self.display_mode,
                    "mode": self.mode,
                    "clock_enabled": self.clock_enabled,
                }
            )

    async def run_media_action(self, command: str) -> None:
        self.audio.send_media_command(command)
        if command == "play_pause":
            self.audio.set_play_state(not self.audio.is_playing)
            await self.send_command("PLAY" if self.audio.is_playing else "PAUSE")
            self.emit_state()

    def load_config(self):
        if not self.config_path.exists():
            self.save_config()
            return
        with self.config_path.open("r", encoding="utf-8") as f:
            data = json.load(f)
        self.button_map = {int(k): v for k, v in data.get("button_map", {}).items()}
        self.display_mode = data.get("display_mode", DEFAULT_DISPLAY_MODE)
        if self.display_mode not in {"volume", "fft", "wave"}:
            self.display_mode = DEFAULT_DISPLAY_MODE

    def save_config(self):
        payload = {
            "button_map": self.button_map,
            "display_mode": self.display_mode,
        }
        with self.config_path.open("w", encoding="utf-8") as f:
            json.dump(payload, f, indent=2)

    async def discover(self):
        self.log("Scanning for ESP32-C3...")
        try:
            devices = await BleakScanner.discover(timeout=5)
        except BleakError as exc:
            self.log(f"BLE scan failed: {exc}")
            self.log(
                "If this is Windows, run the script from a normal console window "
                "and make sure Bluetooth is enabled."
            )
            return None

        discovered = []
        for d in devices:
            name = d.name or ""
            discovered.append(f"{name} ({d.address})")
            uuids = []
            metadata = getattr(d, "metadata", None)
            if metadata is not None and hasattr(metadata, "get"):
                uuids = metadata.get("uuids", [])
            elif hasattr(d, "details"):
                uuids = getattr(d.details, "service_uuids", []) or []

            service_match = any(
                str(uuid).lower() == SERVICE_UUID.lower() for uuid in uuids
            )
            if service_match or (name and "Godhand" in name):
                self.device = d
                self.log(f"Found device: {name or 'Unknown'} ({d.address})")
                return d

        self.log("No matching BLE device found.")
        if discovered:
            self.log("Discovered devices:")
            for item in discovered:
                self.log(f"  - {item}")
        return None

    async def connect(self):
        if self.device is None:
            await self.discover()
        if self.device is None:
            return False

        address = self.device.address
        self.client = BleakClient(address)
        try:
            await self.client.connect()
        except Exception as exc:
            self.log(f"Connection failed to {address}: {exc}")
            return False
        self.connected = True
        self.log(f"Connected to BLE device: {address}")
        self.emit_state()
        return True

    async def disconnect(self):
        self.running = False
        if self.client is not None and self.client.is_connected:
            await self.client.disconnect()
        self.connected = False
        self.emit_state()

    async def send_command(self, command: str):
        if self.client is None or not self.client.is_connected:
            self.log(f"Cannot send, client not connected: {command}")
            return
        data = command.encode("utf-8")
        # Try write without response first (WRITE_NR), fallback to response=True
        try:
            await self.client.write_gatt_char(CMD_UUID, data, response=False)
            self.log(f"Sent (no response): {command}")
            return
        except Exception as e:
            self.log(f"Write without response failed: {e}; trying with response")
        try:
            await self.client.write_gatt_char(CMD_UUID, data, response=True)
            self.log(f"Sent (with response): {command}")
        except Exception as e:
            self.log(f"Failed to send command '{command}': {e}")

    async def read_state(self):
        if self.client is None or not self.client.is_connected:
            return
        value = await self.client.read_gatt_char(STATE_UUID)
        text = value.decode("utf-8", errors="ignore")
        self.log(f"State: {text}")
        for line in text.splitlines():
            self.handle_state_line(line.strip())
        self.emit_state()
        return text

    async def handle_notify(self, sender, data):
        text = data.decode("utf-8", errors="ignore")
        self.log(f"Received: {text}")
        for line in text.splitlines() or [text]:
            self.handle_state_line(line.strip())
        self.emit_state()

    def handle_state_line(self, line: str):
        if not line:
            return
        if line.startswith("VOL:"):
            value = int(line.split(":", 1)[1])
            self.audio.update_volume(value / 100.0)
        elif line.startswith("STATE:"):
            state = line.split(":", 1)[1].strip()
            self.audio.set_play_state(state.upper() == "PLAY")
        elif line.startswith("DISP:"):
            self.display_mode = line.split(":", 1)[1].strip().lower()
        elif line.startswith("MODE:"):
            # firmware sends a single-char mode; reflect it
            self.mode = line.split(":", 1)[1].strip()
        # MODE handled above to update controller.mode
        elif line.startswith("EVENT:"):
            event = line.split(":", 1)[1].strip()
            if event.startswith("BTN"):
                button_id = int(event[3:])
                command = self.button_map.get(button_id)
                if command:
                    asyncio.create_task(self.run_media_action(command))
            elif event.startswith("ENC:"):
                self.log(f"Encoder event: {event}")
            elif event in {"PLAY", "PAUSE"}:
                self.audio.send_media_command("play_pause")
                self.audio.set_play_state(event == "PLAY")
            elif event == "PLAY_PAUSE":
                asyncio.create_task(self.run_media_action("play_pause"))

    async def start_notify(self):
        if self.client is None or not self.client.is_connected:
            return
        await self.client.start_notify(STATE_UUID, self.handle_notify)

    def detect_pitch(self, audio: np.ndarray) -> int:
        if len(audio) == 0:
            return 0

        audio = audio - np.mean(audio)
        windowed = audio * np.hanning(len(audio))
        spectrum = np.abs(np.fft.rfft(windowed))
        freqs = np.fft.rfftfreq(len(audio), 1 / self.sample_rate)

        mask = (freqs >= MIN_FREQ) & (freqs <= MAX_FREQ)

        if not np.any(mask):
            return 0

        peak = np.argmax(spectrum[mask])
        return int(freqs[mask][peak])

    def freq_to_note(self, freq: int) -> str:
        if freq <= 0:
            return "-"

        notes = [
            "C", "C#", "D", "D#", "E", "F",
            "F#", "G", "G#", "A", "A#", "B"
        ]

        midi = round(69 + 12 * np.log2(freq / 440))
        octave = midi // 12 - 1

        return f"{notes[midi % 12]}{octave}"

    def _fft_to_bars(self, spectrum: np.ndarray, bars: int = FFT_BARS) -> list[int]:
        # magnitude and log scaling
        mag = np.abs(spectrum)
        mag = np.log1p(mag)

        # interpolate to desired number of bars
        interp = np.interp(
            np.linspace(0, len(mag) - 1, bars),
            np.arange(len(mag)),
            mag,
        )

        # normalize
        denom = interp.max() if interp.max() > 0 else 1.0
        norm = (interp / denom) * 100.0

        # smooth across frames with exponential moving average
        self._visual_smooth = (0.75 * self._visual_smooth) + (0.25 * norm)
        return np.clip(self._visual_smooth, 0, 100).astype(int).tolist()

    def _wave_to_points(self, audio: np.ndarray, points: int = FFT_BARS) -> list[int]:
        if len(audio) == 0:
            return [50] * points

        # downsample and apply soft clipping/normalization
        indexes = np.linspace(0, len(audio) - 1, points).astype(int)
        sampled = audio[indexes]
        # small DC removal
        sampled = sampled - np.mean(sampled)
        peak = float(np.max(np.abs(sampled))) or 1.0
        normalized = np.clip((sampled / peak) * 0.5 + 0.5, 0.0, 1.0)

        # smooth with previous visual buffer
        vals = (self._visual_smooth[:points] * 0.75) + (normalized * 100 * 0.25)
        self._visual_smooth[:points] = vals
        return np.clip(vals, 0, 100).astype(int).tolist()

    def _pack_visual_values(self, values: list[int]) -> str:
        packed = []
        for value in values[:FFT_BARS]:
            index = round(max(0, min(100, int(value))) * (len(VISUAL_ALPHABET) - 1) / 100)
            packed.append(VISUAL_ALPHABET[index])
        return "".join(packed).ljust(FFT_BARS, "A")

    def callback(self, indata, frames, time_info, status):
        if status:
            self.log(str(status))
        audio = np.asarray(indata, dtype=np.float32)
        if audio.ndim > 1:
            audio = audio.mean(axis=1)
        audio = audio.reshape(-1)
        audio = audio[: self.chunk_size]
        if len(audio) < self.chunk_size:
            audio = np.pad(audio, (0, self.chunk_size - len(audio)), mode="constant")
        # apply window before FFT for better spectral shape
        if self.display_mode == "wave":
            visual_mode = "W"
            values = self._wave_to_points(audio)
        elif self.display_mode == "fft":
            visual_mode = "F"
            windowed = audio * np.hanning(len(audio))
            spectrum = np.fft.rfft(windowed)
            values = self._fft_to_bars(spectrum)

            freq = self.detect_pitch(audio)
            note = self.freq_to_note(freq)

            if self.loop is not None:
                asyncio.run_coroutine_threadsafe(
                    self.send_visual_values_with_note(
                        visual_mode,
                        values,
                        note,
                        freq
                    ),
                    self.loop,
                )
                return
        else:
            return
        try:
            if self.loop is not None:
                asyncio.run_coroutine_threadsafe(
                    self.send_visual_values(visual_mode, values),
                    self.loop,
                )
        except Exception as e:
            self.log(f"Failed to send visual values: {e}")

    async def send_fft_bars(self, bars):
        payload = ",".join(str(v) for v in bars)
        await self.send_command(f"FFT:{payload}")

    async def send_visual_values(self, visual_mode: str, values: list[int]):
        payload = self._pack_visual_values(values)
        await self.send_command(f"B:{visual_mode}:{payload}")

    async def send_visual_values_with_note(
        self,
        visual_mode: str,
        values: list[int],
        note: str,
        freq: int
    ):
        payload = self._pack_visual_values(values)
        await self.send_command(
            f"B:{visual_mode}:{payload}:N:{note}:{freq}"
        )

    async def sync_time(self):
        if not self.clock_enabled:
            return
        await self.send_command(f"TIME:{datetime.now().strftime('%H:%M:%S')}")

    async def sync_time_loop(self):
        while self.running:
            await self.sync_time()
            await asyncio.sleep(TIME_SYNC_INTERVAL_SECONDS)

    async def stream_audio(self):
        if sys.platform == "win32":
            soundcard_streamed = await self._stream_windows_loopback_soundcard()
            if soundcard_streamed:
                return
        sounddevice_module = _get_sounddevice()
        if sounddevice_module is None:
            raise RuntimeError("sounddevice is not installed")
        stream_kwargs = self._audio_stream_kwargs(sounddevice_module)
        with sounddevice_module.InputStream(**stream_kwargs):
            while self.running:
                await asyncio.sleep(0.1)

    def _audio_stream_kwargs(self, sounddevice_module) -> dict:
        kwargs = {
            "samplerate": self.sample_rate,
            "channels": 1,
            "dtype": "float32",
            "blocksize": self.chunk_size,
            "callback": self.callback,
        }
        if sys.platform == "win32":
            try:
                loopback_device = self._find_windows_loopback_device(sounddevice_module)
                if loopback_device is not None:
                    device_index, device_info = loopback_device
                    kwargs["device"] = device_index
                    kwargs["channels"] = min(2, int(device_info.get("max_input_channels", 1)) or 1)
                    kwargs["samplerate"] = int(device_info.get("default_samplerate", self.sample_rate))
                    self.log(f"Audio source: {device_info.get('name', 'Windows loopback')}")
                else:
                    self.log("Loopback audio device not found, using default input")
            except Exception as exc:
                self.log(f"Loopback audio unavailable, using default input: {exc}")
        else:
            self.log("Audio source: default input device")
        return kwargs

    async def _stream_windows_loopback_soundcard(self) -> bool:
        try:
            import soundcard as sc  # type: ignore
        except Exception:
            self.log("soundcard is not installed; trying sounddevice input")
            return False

        try:
            speaker = sc.default_speaker()
            microphone = sc.get_microphone(speaker.name, include_loopback=True)
            self.log(f"Audio source: {speaker.name} loopback")
            with microphone.recorder(samplerate=self.sample_rate, channels=2) as recorder:
                while self.running:
                    audio = await asyncio.to_thread(recorder.record, numframes=self.chunk_size)
                    self.callback(audio, self.chunk_size, None, None)
            return True
        except Exception as exc:
            self.log(f"soundcard loopback unavailable: {exc}")
            return False

    def _find_windows_loopback_device(self, sounddevice_module):
        hostapis = sounddevice_module.query_hostapis()
        devices = sounddevice_module.query_devices()
        preferred_words = ("loopback", "stereo mix", "what u hear", "output")
        for index, device in enumerate(devices):
            if int(device.get("max_input_channels", 0)) <= 0:
                continue
            hostapi = hostapis[device.get("hostapi", -1)] if device.get("hostapi", -1) >= 0 else {}
            hostapi_name = str(hostapi.get("name", "")).lower()
            device_name = str(device.get("name", "")).lower()
            if "wasapi" in hostapi_name and any(word in device_name for word in preferred_words):
                return index, device
        return None

    async def run(self):
        self.loop = asyncio.get_running_loop()
        if not await self.connect():
            self.connected = False
            self.emit_state()
            return
        await self.start_notify()
        await self.read_state()
        await self.send_command("MODE:A")
        await self.send_command(f"DISP:{self.display_mode}")
        await self.send_command(f"CLOCK:{'on' if self.clock_enabled else 'off'}")
        await self.sync_time()

        time_task = asyncio.create_task(self.sync_time_loop())

        try:
            if self.disable_audio:
                while self.running:
                    await asyncio.sleep(0.5)
            else:
                await self.stream_audio()
        except Exception as exc:
            self.log(f"Audio stream failed: {exc}")
            self.log("Continuing in command-only mode.")
            while self.running:
                await asyncio.sleep(0.5)
        finally:
            time_task.cancel()
            self.connected = False
            self.emit_state()


class ControllerGui:
    def __init__(self, config_path: Path):
        import tkinter as tk
        from tkinter import scrolledtext, ttk

        self.tk = tk
        self.root = tk.Tk()
        self.root.title("Godhand Controller")
        self.root.geometry("560x420")
        self.root.minsize(480, 360)
        self.controller: Optional[BLEMusicController] = None
        self.thread: Optional[threading.Thread] = None
        self.config_path = config_path

        self.address_var = tk.StringVar()
        self.status_var = tk.StringVar(value="Disconnected")
        self.mode_var = tk.StringVar(value="A")
        self.volume_var = tk.IntVar(value=50)
        self.play_var = tk.StringVar(value="PLAY")
        self.display_var = tk.StringVar(value=DEFAULT_DISPLAY_MODE)
        self.clock_var = tk.BooleanVar(value=True)
        self.audio_var = tk.BooleanVar(value=False)

        root = self.root
        root.columnconfigure(0, weight=1)
        root.rowconfigure(4, weight=1)

        top = ttk.Frame(root, padding=12)
        top.grid(row=0, column=0, sticky="ew")
        top.columnconfigure(1, weight=1)
        ttk.Label(top, text="BLE Address").grid(row=0, column=0, sticky="w")
        ttk.Entry(top, textvariable=self.address_var).grid(row=0, column=1, sticky="ew", padx=8)
        self.connect_button = ttk.Button(top, text="Connect", command=self.connect)
        self.connect_button.grid(row=0, column=2)
        self.disconnect_button = ttk.Button(top, text="Disconnect", command=self.disconnect, state="disabled")
        self.disconnect_button.grid(row=0, column=3, padx=(8, 0))

        status = ttk.Frame(root, padding=(12, 0, 12, 8))
        status.grid(row=1, column=0, sticky="ew")
        status.columnconfigure(1, weight=1)
        ttk.Label(status, textvariable=self.status_var).grid(row=0, column=0, sticky="w")
        ttk.Label(status, textvariable=self.play_var).grid(row=0, column=1, sticky="e")
        ttk.Label(status, text="Mode:").grid(row=0, column=2, sticky="e", padx=(8,0))
        ttk.Label(status, textvariable=self.mode_var).grid(row=0, column=3, sticky="e")

        controls = ttk.Frame(root, padding=(12, 0, 12, 8))
        controls.grid(row=2, column=0, sticky="ew")
        controls.columnconfigure(1, weight=1)
        ttk.Label(controls, text="Volume").grid(row=0, column=0, sticky="w")
        self.volume_scale = ttk.Scale(controls, from_=0, to=100, variable=self.volume_var, command=self.set_volume)
        self.volume_scale.grid(row=0, column=1, sticky="ew", padx=8)
        ttk.Button(controls, text="Play/Pause", command=self.toggle_playback).grid(row=0, column=2)
        ttk.Button(controls, text="Mode", command=self.cycle_mode).grid(row=0, column=3)
        ttk.Button(controls, text="Sync Time", command=self.sync_time).grid(row=0, column=4, padx=(8, 0))

        options = ttk.Frame(root, padding=(12, 0, 12, 8))
        options.grid(row=3, column=0, sticky="ew")
        ttk.Label(options, text="Display").grid(row=0, column=0, sticky="w")
        ttk.Combobox(
            options,
            textvariable=self.display_var,
            values=("volume", "fft", "wave"),
            width=10,
            state="readonly",
        ).grid(row=0, column=1, padx=8)
        ttk.Button(options, text="Apply", command=self.apply_display).grid(row=0, column=2)
        ttk.Checkbutton(options, text="Clock", variable=self.clock_var, command=self.apply_clock).grid(
            row=0, column=3, padx=(16, 0)
        )
        ttk.Checkbutton(options, text="No audio capture", variable=self.audio_var).grid(row=0, column=4, padx=(16, 0))

        self.log_box = scrolledtext.ScrolledText(root, height=10, wrap="word")
        self.log_box.grid(row=4, column=0, sticky="nsew", padx=12, pady=(0, 12))
        self.log_box.configure(state="disabled")
        self.root.protocol("WM_DELETE_WINDOW", self.close)

    def append_log(self, message: str) -> None:
        self.root.after(0, self._append_log, message)

    def _append_log(self, message: str) -> None:
        self.log_box.configure(state="normal")
        self.log_box.insert("end", message + "\n")
        self.log_box.see("end")
        self.log_box.configure(state="disabled")

    def update_state(self, state: dict) -> None:
        self.root.after(0, self._update_state, state)

    def _update_state(self, state: dict) -> None:
        self.status_var.set("Connected" if state["connected"] else "Disconnected")
        # Temporarily disable the volume scale command to avoid recursion
        old_command = self.volume_scale.configure('command')[4]
        self.volume_scale.configure(command='')
        self.volume_var.set(state["volume"])
        self.volume_scale.configure(command=old_command)
        self.play_var.set("PLAY" if state["playing"] else "PAUSE")
        self.display_var.set(state["display_mode"])
        # update mode display if provided
        if "mode" in state:
            self.mode_var.set(state["mode"])
        self.clock_var.set(state["clock_enabled"])
        self.connect_button.configure(state="disabled" if state["connected"] else "normal")
        self.disconnect_button.configure(state="normal" if state["connected"] else "disabled")

    def connect(self) -> None:
        if self.thread and self.thread.is_alive():
            return
        self.controller = BLEMusicController(
            config_path=self.config_path,
            disable_audio=self.audio_var.get(),
            clock_enabled=self.clock_var.get(),
        )
        self.controller.on_log = self.append_log
        self.controller.on_state = self.update_state
        address = self.address_var.get().strip()
        if address:
            self.controller.device = type("Device", (), {"address": address, "name": "Manual"})()
        self.thread = threading.Thread(target=lambda: asyncio.run(self.controller.run()), daemon=True)
        self.thread.start()
        self.status_var.set("Connecting...")
        self.connect_button.configure(state="disabled")

    def disconnect(self) -> None:
        if self.controller and self.controller.loop:
            asyncio.run_coroutine_threadsafe(self.controller.disconnect(), self.controller.loop)

    def send(self, command: str) -> None:
        # Log send action in GUI
        self.append_log(f"GUI send: {command}")
        if self.controller and self.controller.loop:
            asyncio.run_coroutine_threadsafe(self.controller.send_command(command), self.controller.loop)

    def set_volume(self, _value: str) -> None:
        volume_value = int(self.volume_var.get())
        # Update local audio controller volume
        if self.controller:
            self.controller.audio.update_volume(volume_value / 100.0)
        # Send command to device
        self.send(f"VOL:{volume_value}")

    def sync_time(self) -> None:
        if self.controller and self.controller.loop:
            asyncio.run_coroutine_threadsafe(self.controller.sync_time(), self.controller.loop)

    def cycle_mode(self) -> None:
        # Cycle mode A->B->C->A and send to device
        current = 'A'
        if self.controller:
            current = getattr(self.controller, 'mode', 'A') or 'A'
        next_mode = 'A'
        if current == 'A':
            next_mode = 'B'
        elif current == 'B':
            next_mode = 'C'
        else:
            next_mode = 'A'
        self.send(f"MODE:{next_mode}")

    def toggle_playback(self) -> None:
        if self.controller and self.controller.loop:
            asyncio.run_coroutine_threadsafe(
                self.controller.run_media_action("play_pause"),
                self.controller.loop,
            )

    def apply_display(self) -> None:
        if self.controller:
            self.controller.display_mode = self.display_var.get()
            self.controller.save_config()
        self.send(f"DISP:{self.display_var.get()}")

    def apply_clock(self) -> None:
        enabled = self.clock_var.get()
        if self.controller:
            self.controller.clock_enabled = enabled
        self.send(f"CLOCK:{'on' if enabled else 'off'}")
        if enabled:
            self.sync_time()

    def run(self) -> None:
        self.root.mainloop()

    def close(self) -> None:
        if self.controller:
            self.controller.on_log = None
            self.controller.on_state = None
        self.disconnect()
        self.root.destroy()


async def main():
    parser = argparse.ArgumentParser(description="BLE controller for the Godhand device")
    parser.add_argument("--config", default="controller_config.json", help="Path to JSON config file")
    parser.add_argument("--no-audio", action="store_true", help="Disable audio capture and only use BLE controls")
    parser.add_argument("--clock-off", action="store_true", help="Disable the clock on the OLED display")
    parser.add_argument("--gui", action="store_true", help="Launch the desktop GUI")
    parser.add_argument(
        "--address",
        default=None,
        help="Manually specify the BLE device address to connect to",
    )
    args = parser.parse_args()

    if args.gui:
        ControllerGui(Path(args.config)).run()
        return

    controller = BLEMusicController(
        config_path=Path(args.config),
        disable_audio=args.no_audio,
        clock_enabled=not args.clock_off,
    )
    if args.address:
        controller.device = type("Device", (), {"address": args.address, "name": "Manual"})()

    await controller.run()


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("Stopped.")
        sys.exit(0)
