from __future__ import annotations

import asyncio
import logging
import math
import threading
import time
from datetime import datetime

from PySide6.QtCore import QObject, QTimer, Signal

from app.config import SettingsStore
from app.state import AppState, AudioActivity, ConnectionStatus
from audio.activity import AudioActivityDetector
from audio.capture import AudioCapture
from audio.fft import FFTAnalyzer
from gui.main_window import MainWindow
from protocol.parser import parse_line
from system.media import MediaController
from system.volume import VolumeController
from transport.base import DeviceTransport


class TanaControllerApp(QObject):
    state_changed = Signal(object)
    device_message_received = Signal(str)
    device_sync_requested = Signal()

    def __init__(self, settings: SettingsStore):
        super().__init__()
        self.settings = settings
        self.state = AppState()
        self.volume = VolumeController()
        self.media = MediaController()
        self.audio_capture = AudioCapture()
        self.transport: DeviceTransport | None = None
        self.transport_loop: asyncio.AbstractEventLoop | None = None
        self.transport_thread: threading.Thread | None = None
        self._last_visual_payload = ""
        self._last_visual_values: list[int] = []
        self._last_visual_sent_at = 0.0
        self._last_audible_at = time.monotonic()
        self._last_sound_state_sent: bool | None = None
        self.fft = FFTAnalyzer(
            bands=int(settings.get("audio.fft_bands", 16)),
            smoothing=float(settings.get("audio.fft_smoothing", 0.75)),
            sensitivity=float(settings.get("audio.fft_sensitivity", 1.0)),
        )
        self.activity = AudioActivityDetector(
            silence_threshold=float(settings.get("audio.silence_threshold", 0.02)),
            silence_seconds=float(settings.get("audio.silence_detection_seconds", 5.0)),
            active_seconds=float(settings.get("audio.audio_detection_seconds", 1.0)),
        )
        self.window = MainWindow(self.state, settings)
        self.window.connect_requested.connect(self.connect)
        self.window.disconnect_requested.connect(self.disconnect)
        self.window.command_requested.connect(self.send_command)
        self.window.volume_requested.connect(self.set_volume)
        self.window.setting_changed.connect(self.change_setting)
        self.state_changed.connect(self.window.update_state)
        self.device_message_received.connect(self._handle_device_message)
        self.device_sync_requested.connect(self.sync_device_state)

        self.demo_timer = QTimer(self)
        self.demo_timer.timeout.connect(self._update_demo_fft)
        self.demo_timer.start(max(66, int(1000 / min(15, int(settings.get("audio.fft_update_rate", 30))))))

        self.sync_timer = QTimer(self)
        self.sync_timer.timeout.connect(self.sync_device_state)
        self.sync_timer.start(30_000)

        self.refresh_volume()

    def show(self) -> None:
        self.window.show()
        if self.settings.get("device.auto_connect", True):
            self.connect()

    def connect(self) -> None:
        if self.transport_thread is not None and self.transport_thread.is_alive():
            return

        self.state.esp32.status = ConnectionStatus.SCANNING
        self.state.esp32.connection_type = "BLE"
        self.state_changed.emit(self.state)

        self.transport_thread = threading.Thread(target=self._run_ble_worker, daemon=True)
        self.transport_thread.start()

    def disconnect(self) -> None:
        if self.transport_loop is not None and self.transport is not None:
            asyncio.run_coroutine_threadsafe(self._disconnect_ble(), self.transport_loop)
        self.audio_capture.close()
        self.state.esp32.status = ConnectionStatus.DISCONNECTED
        self.state.esp32.last_packet = "-"
        self._log("Disconnected.")
        self.state_changed.emit(self.state)

    def send_command(self, action: str) -> None:
        command = ""
        if action == "play_pause":
            self.media.play_pause()
            command = "PLAY_PAUSE"
        elif action == "next":
            self.media.next()
            command = "EVENT:NEXT"
        elif action == "previous":
            self.media.previous()
            command = "EVENT:PREVIOUS"
        elif action == "mute":
            muted = self.volume.mute()
            self.state.audio.muted = muted
            command = f"MUTE:{1 if muted else 0}"
        else:
            command = action

        self._send_to_device(command)
        self.state.esp32.last_packet = command
        self._log(f"Queued command: {command}")
        self.state_changed.emit(self.state)

    def set_volume(self, value: int) -> None:
        self.volume.set(value / 100.0)
        self.volume.set_mute(False)
        self.state.audio.volume = value
        self.state.audio.muted = False
        command = f"VOL:{value}"
        self._send_to_device(command)
        self.state.esp32.last_packet = command
        self._log(f"Volume set: {value}%")
        self.state_changed.emit(self.state)

    def change_setting(self, key: str, value: object) -> None:
        self.settings.set(key, value)
        if key == "audio.fft_enabled":
            self.state.audio.fft_enabled = bool(value)
        if key == "audio.fft_bands":
            self.fft.set_bands(int(value))
            self.state.audio.bars = [0] * int(value)
        if key == "display.default_screen":
            self._sync_display_mode()
        self._log(f"Setting changed: {key}={value}")
        self.state_changed.emit(self.state)

    def refresh_volume(self) -> None:
        current = self.volume.get()
        if current is not None:
            self.state.audio.volume = int(current * 100)
        self.state.audio.muted = self.volume.get_mute()
        if current is not None:
            self.state_changed.emit(self.state)

    def _update_demo_fft(self) -> None:
        if not self.state.audio.fft_enabled:
            return

        now = time.monotonic()
        previous_playing = self.state.audio.playing

        samples = self.audio_capture.record_once()
        peak = float(abs(samples).max()) if samples.size else 0.0
        rms = float((samples.astype("float32") ** 2).mean() ** 0.5) if samples.size else 0.0
        level = max(peak, rms * 4.0)
        audible_threshold = float(self.settings.get("audio.silence_threshold", 0.02)) * 0.15
        audible = level >= max(0.0015, audible_threshold)
        if audible:
            self._last_audible_at = now
        if level > 0.00005:
            values = self.fft.analyze(samples, self.audio_capture.sample_rate)
        else:
            values = [
                int((math.sin(now * 2.2 + index * 0.55) * 0.5 + 0.5) * 100)
                for index in range(self.fft.bands)
            ]
            if not self.state.audio.playing:
                values = [0] * self.fft.bands

        activity = self.activity.update(level, now)
        self.state.audio.activity = AudioActivity(activity.value)
        if audible:
            self.state.audio.playing = True
        elif now - self._last_audible_at >= 8.0:
            self.state.audio.playing = False

        self.state.audio.bars = values
        self.state.audio.fft_fps = int(self.settings.get("audio.fft_update_rate", 30))
        if previous_playing != self.state.audio.playing or self._last_sound_state_sent != self.state.audio.playing:
            self._send_to_device("PLAY" if self.state.audio.playing else "PAUSE")
            self._sync_display_mode()
            self._last_sound_state_sent = self.state.audio.playing
            if not self.state.audio.playing:
                self._last_visual_payload = ""
        if self._target_display_mode() == "disc":
            self._send_visual_values("D", values)
        self.state_changed.emit(self.state)

    def sync_device_state(self) -> None:
        current = self.volume.get()
        if current is not None:
            self.state.audio.volume = int(current * 100)
        self.state.audio.muted = self.volume.get_mute()

        self._send_to_device(f"TIME:{datetime.now().strftime('%H:%M:%S')}")
        self._send_to_device(f"VOL:{self.state.audio.volume}")
        self._send_to_device(f"MUTE:{1 if self.state.audio.muted else 0}")
        self._send_to_device("PLAY" if self.state.audio.playing else "PAUSE")
        self._sync_display_mode()
        self.state_changed.emit(self.state)

    def _log(self, message: str) -> None:
        logging.info(message)
        self.state.add_log(message)

    def _run_ble_worker(self) -> None:
        loop = asyncio.new_event_loop()
        self.transport_loop = loop
        asyncio.set_event_loop(loop)
        loop.run_until_complete(self._connect_ble())
        loop.run_forever()
        loop.close()
        self.transport = None
        self.transport_loop = None
        self.transport_thread = None

    async def _connect_ble(self) -> None:
        try:
            from transport.ble import BleTransport
        except ModuleNotFoundError as exc:
            self.state.esp32.status = ConnectionStatus.ERROR
            self._log(f"BLE dependency missing: {exc.name}. Run python -m pip install -r requirements.txt")
            self.state_changed.emit(self.state)
            if self.transport_loop is not None:
                self.transport_loop.call_soon_threadsafe(self.transport_loop.stop)
            return

        transport = BleTransport(str(self.settings.get("device.esp32_name", "Godhand Controller")))
        transport.on_message = self.device_message_received.emit
        self.transport = transport

        self.state.esp32.status = ConnectionStatus.CONNECTING
        self.state_changed.emit(self.state)
        try:
            connected = await transport.connect()
        except Exception as exc:
            self.state.esp32.status = ConnectionStatus.ERROR
            self._log(f"BLE connect failed: {exc}")
            self.state_changed.emit(self.state)
            return

        if connected:
            self.state.esp32.status = ConnectionStatus.CONNECTED
            self._log("BLE connected.")
            self.device_sync_requested.emit()
        else:
            self.state.esp32.status = ConnectionStatus.DISCONNECTED
            self._log("BLE device not found.")
        self.state_changed.emit(self.state)

    async def _disconnect_ble(self) -> None:
        if self.transport is not None:
            await self.transport.disconnect()
        if self.transport_loop is not None:
            self.transport_loop.call_soon_threadsafe(self.transport_loop.stop)

    def _send_to_device(self, command: str) -> None:
        if self.transport_loop is None or self.transport is None:
            return
        asyncio.run_coroutine_threadsafe(self.transport.send(command), self.transport_loop)

    def _handle_device_message(self, message: str) -> None:
        for line in message.splitlines():
            parsed = parse_line(line)
            if parsed is None:
                continue
            kind, payload = parsed
            if kind == "VOL":
                try:
                    self.state.audio.volume = max(0, min(100, int(payload)))
                except ValueError:
                    continue
                self.volume.set(self.state.audio.volume / 100.0)
            elif kind == "STATE":
                self.state.esp32.last_packet = f"STATE:{payload}"
            elif kind == "MODE":
                self.state.esp32.last_packet = f"MODE:{payload}"
            elif kind == "DISP":
                self.state.esp32.last_packet = f"DISP:{payload}"
            elif kind == "MUTE":
                muted = payload in {"1", "on", "true", "ON", "TRUE"}
                self.volume.set_mute(muted)
                self.state.audio.muted = muted
                self.state.esp32.last_packet = f"MUTE:{payload}"
            elif kind == "EVENT":
                self.state.esp32.last_packet = f"EVENT:{payload}"
                self._handle_device_event(payload)
        self.state_changed.emit(self.state)

    def _handle_device_event(self, event: str) -> None:
        if event.startswith("BTN"):
            try:
                button_id = int(event[3:])
            except ValueError:
                return
            action = str(self.settings.get(f"controls.esp_buttons.{button_id}", ""))
            if action == "play_pause":
                self.media.play_pause()
            elif action == "previous":
                self.media.previous()
            elif action == "next":
                self.media.next()
            elif action == "mute":
                muted = self.volume.mute()
                self.state.audio.muted = muted
                self._send_to_device(f"MUTE:{1 if muted else 0}")
            return

        if event.startswith("ENC:"):
            self._handle_encoder_event(event)
            return

        if event == "PLAY_PAUSE" or event in {"PLAY", "PAUSE"}:
            self.media.play_pause()

    def _handle_encoder_event(self, event: str) -> None:
        parts = event.split(":")
        try:
            steps = int(parts[1] if len(parts) > 1 else event[4:])
        except (ValueError, IndexError):
            return

        step_size = 5
        if len(parts) > 2:
            try:
                next_volume = max(0, min(100, int(parts[2])))
            except ValueError:
                return
        else:
            current = self.volume.get()
            base = self.state.audio.volume if current is None else int(current * 100)
            next_volume = max(0, min(100, base + steps * step_size))
        self.volume.set(next_volume / 100.0)
        self.volume.set_mute(False)
        self.state.audio.volume = next_volume
        self.state.audio.muted = False
        self._send_to_device(f"VOL:{next_volume}")
        self.state.esp32.last_packet = f"EVENT:{event}"
        self._log(f"Encoder volume: {next_volume}%")

    def _target_display_mode(self) -> str:
        configured = str(self.settings.get("display.default_screen", "AUTO")).upper()
        if configured == "AUTO":
            return "disc" if self.state.audio.playing else "clock"
        if configured == "DISC" or configured == "DISK":
            return "disc"
        if configured == "CLOCK":
            return "clock"
        self.settings.set("display.default_screen", "AUTO")
        return "disc" if self.state.audio.playing else "clock"

    def _sync_display_mode(self) -> None:
        self._send_to_device(f"DISP:{self._target_display_mode()}")

    def _send_visual_values(self, visual_mode: str, values: list[int]) -> None:
        alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"
        source = values or [0]
        grouped = []
        for index in range(3):
            start = (len(source) * index) // 3
            end = max(start + 1, (len(source) * (index + 1)) // 3)
            bucket = source[start:end]
            grouped.append(int(sum(bucket) / len(bucket)))
        now = time.monotonic()
        if now - self._last_visual_sent_at < 0.066:
            return
        if self._last_visual_values and max(abs(a - b) for a, b in zip(grouped, self._last_visual_values)) < 4:
            return
        payload = "".join(
            alphabet[round(max(0, min(100, int(value))) * (len(alphabet) - 1) / 100)]
            for value in grouped
        )
        message = f"B:{visual_mode}:{payload}"
        if message == self._last_visual_payload:
            return
        self._last_visual_values = grouped
        self._last_visual_sent_at = now
        self._last_visual_payload = message
        self._send_to_device(message)
