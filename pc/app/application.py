from __future__ import annotations

import logging
import math
import time

from PySide6.QtCore import QObject, QTimer, Signal

from app.config import SettingsStore
from app.state import AppState, AudioActivity, ConnectionStatus
from audio.activity import AudioActivityDetector
from audio.fft import FFTAnalyzer
from gui.main_window import MainWindow
from protocol.encoder import encode_command
from system.media import MediaController
from system.volume import VolumeController


class TanaControllerApp(QObject):
    state_changed = Signal(object)

    def __init__(self, settings: SettingsStore):
        super().__init__()
        self.settings = settings
        self.state = AppState()
        self.volume = VolumeController()
        self.media = MediaController()
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

        self.demo_timer = QTimer(self)
        self.demo_timer.timeout.connect(self._update_demo_fft)
        self.demo_timer.start(max(16, int(1000 / int(settings.get("audio.fft_update_rate", 30)))))

        self.refresh_volume()

    def show(self) -> None:
        self.window.show()
        if self.settings.get("device.auto_connect", True):
            self.connect()

    def connect(self) -> None:
        self.state.esp32.status = ConnectionStatus.SCANNING
        self.state.esp32.connection_type = "BLE"
        self._log("BLE transport scaffold ready; real async worker is next.")
        self.state_changed.emit(self.state)

    def disconnect(self) -> None:
        self.state.esp32.status = ConnectionStatus.DISCONNECTED
        self.state.esp32.last_packet = "-"
        self._log("Disconnected.")
        self.state_changed.emit(self.state)

    def send_command(self, action: str) -> None:
        command = encode_command(action)
        if action == "play_pause":
            self.media.play_pause()
            self.state.audio.playing = not self.state.audio.playing
        elif action == "next":
            self.media.next()
        elif action == "previous":
            self.media.previous()
        elif action == "mute":
            self.volume.mute()
        self.state.esp32.last_packet = command
        self._log(f"Queued command: {command}")
        self.state_changed.emit(self.state)

    def set_volume(self, value: int) -> None:
        self.volume.set(value / 100.0)
        self.state.audio.volume = value
        self.state.esp32.last_packet = f"VOL:{value}"
        self._log(f"Volume set: {value}%")
        self.state_changed.emit(self.state)

    def change_setting(self, key: str, value: object) -> None:
        self.settings.set(key, value)
        if key == "audio.fft_bands":
            self.fft.set_bands(int(value))
            self.state.audio.bars = [0] * int(value)
        self._log(f"Setting changed: {key}={value}")
        self.state_changed.emit(self.state)

    def refresh_volume(self) -> None:
        current = self.volume.get()
        if current is not None:
            self.state.audio.volume = int(current * 100)
            self.state_changed.emit(self.state)

    def _update_demo_fft(self) -> None:
        if not self.state.audio.fft_enabled:
            return

        now = time.monotonic()
        values = [
            int((math.sin(now * 2.2 + index * 0.55) * 0.5 + 0.5) * 100)
            for index in range(self.fft.bands)
        ]
        level = max(values) / 100.0
        activity = self.activity.update(level, now)
        self.state.audio.activity = AudioActivity(activity.value)
        self.state.audio.bars = values
        self.state.audio.fft_fps = int(self.settings.get("audio.fft_update_rate", 30))
        self.state_changed.emit(self.state)

    def _log(self, message: str) -> None:
        logging.info(message)
        self.state.add_log(message)
