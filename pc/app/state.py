from __future__ import annotations

from dataclasses import dataclass, field
from enum import Enum


class ConnectionStatus(str, Enum):
    DISCONNECTED = "Disconnected"
    SCANNING = "Scanning"
    CONNECTING = "Connecting"
    CONNECTED = "Connected"
    ERROR = "Error"


class AudioActivity(str, Enum):
    SILENT = "Silent"
    CANDIDATE = "Candidate"
    ACTIVE = "Active"


@dataclass
class DeviceState:
    name: str
    status: ConnectionStatus = ConnectionStatus.DISCONNECTED
    connection_type: str = "-"
    last_packet: str = "-"
    latency_ms: int | None = None


@dataclass
class AudioState:
    volume: int = 50
    muted: bool = False
    playing: bool = False
    fft_enabled: bool = True
    fft_fps: int = 30
    activity: AudioActivity = AudioActivity.SILENT
    bars: list[int] = field(default_factory=lambda: [0] * 16)


@dataclass
class AppState:
    esp32: DeviceState = field(default_factory=lambda: DeviceState("ESP32-C3"))
    pico: DeviceState = field(default_factory=lambda: DeviceState("Pico 2"))
    audio: AudioState = field(default_factory=AudioState)
    log_tail: list[str] = field(default_factory=list)

    def add_log(self, message: str) -> None:
        self.log_tail.append(message)
        self.log_tail = self.log_tail[-200:]
