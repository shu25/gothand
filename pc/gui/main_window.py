from __future__ import annotations

from PySide6.QtCore import Signal
from PySide6.QtWidgets import (
    QCheckBox,
    QComboBox,
    QFormLayout,
    QGridLayout,
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QMainWindow,
    QPushButton,
    QProgressBar,
    QSlider,
    QSpinBox,
    QTabWidget,
    QTextEdit,
    QVBoxLayout,
    QWidget,
)
from PySide6.QtCore import Qt

from app.config import SettingsStore
from app.state import AppState


class MainWindow(QMainWindow):
    connect_requested = Signal()
    disconnect_requested = Signal()
    command_requested = Signal(str)
    volume_requested = Signal(int)
    setting_changed = Signal(str, object)

    def __init__(self, state: AppState, settings: SettingsStore):
        super().__init__()
        self.settings = settings
        self.setWindowTitle("Tana Controller")
        self.resize(860, 560)
        self.bars: list[QProgressBar] = []

        tabs = QTabWidget()
        tabs.addTab(self._home_page(), "Home")
        tabs.addTab(self._device_page(), "Device")
        tabs.addTab(self._audio_page(), "Audio")
        tabs.addTab(self._settings_page(), "Settings")
        self.setCentralWidget(tabs)
        self.update_state(state)

    def _home_page(self) -> QWidget:
        page = QWidget()
        layout = QVBoxLayout(page)

        summary = QGridLayout()
        self.esp_status = QLabel()
        self.pico_status = QLabel()
        self.volume_label = QLabel()
        self.media_label = QLabel()
        self.fft_label = QLabel()
        self.latency_label = QLabel()
        labels = [
            ("ESP32-C3", self.esp_status),
            ("Pico 2", self.pico_status),
            ("Volume", self.volume_label),
            ("Media", self.media_label),
            ("FFT", self.fft_label),
            ("Latency", self.latency_label),
        ]
        for row, (title, widget) in enumerate(labels):
            summary.addWidget(QLabel(title), row, 0)
            summary.addWidget(widget, row, 1)
        layout.addLayout(summary)

        fft_box = QGroupBox("FFT Debug")
        fft_layout = QHBoxLayout(fft_box)
        for _ in range(int(self.settings.get("audio.fft_bands", 16))):
            bar = QProgressBar()
            bar.setRange(0, 100)
            bar.setOrientation(Qt.Vertical)
            bar.setTextVisible(False)
            fft_layout.addWidget(bar)
            self.bars.append(bar)
        layout.addWidget(fft_box)

        self.log_view = QTextEdit()
        self.log_view.setReadOnly(True)
        layout.addWidget(self.log_view)
        return page

    def _device_page(self) -> QWidget:
        page = QWidget()
        layout = QVBoxLayout(page)
        buttons = QHBoxLayout()
        connect = QPushButton("Connect")
        disconnect = QPushButton("Disconnect")
        reconnect = QPushButton("Reconnect")
        connect.clicked.connect(self.connect_requested)
        disconnect.clicked.connect(self.disconnect_requested)
        reconnect.clicked.connect(self.connect_requested)
        buttons.addWidget(connect)
        buttons.addWidget(disconnect)
        buttons.addWidget(reconnect)
        layout.addLayout(buttons)

        self.device_detail = QLabel()
        layout.addWidget(self.device_detail)
        layout.addStretch(1)
        return page

    def _audio_page(self) -> QWidget:
        page = QWidget()
        layout = QFormLayout(page)

        self.volume_slider = QSlider(Qt.Horizontal)
        self.volume_slider.setRange(0, 100)
        self.volume_slider.sliderReleased.connect(lambda: self.volume_requested.emit(self.volume_slider.value()))
        layout.addRow("Volume", self.volume_slider)

        self.fft_enabled = QCheckBox()
        self.fft_enabled.setChecked(bool(self.settings.get("audio.fft_enabled", True)))
        self.fft_enabled.toggled.connect(lambda value: self.setting_changed.emit("audio.fft_enabled", value))
        layout.addRow("FFT Enable", self.fft_enabled)

        self.fft_bands = QSpinBox()
        self.fft_bands.setRange(8, 32)
        self.fft_bands.setSingleStep(8)
        self.fft_bands.setValue(int(self.settings.get("audio.fft_bands", 16)))
        self.fft_bands.valueChanged.connect(lambda value: self.setting_changed.emit("audio.fft_bands", value))
        layout.addRow("FFT Bands", self.fft_bands)

        play = QPushButton("Play/Pause")
        previous = QPushButton("Previous")
        next_button = QPushButton("Next")
        mute = QPushButton("Mute")
        play.clicked.connect(lambda: self.command_requested.emit("play_pause"))
        previous.clicked.connect(lambda: self.command_requested.emit("previous"))
        next_button.clicked.connect(lambda: self.command_requested.emit("next"))
        mute.clicked.connect(lambda: self.command_requested.emit("mute"))
        media = QHBoxLayout()
        media.addWidget(previous)
        media.addWidget(play)
        media.addWidget(next_button)
        media.addWidget(mute)
        layout.addRow("Media", media)
        return page

    def _settings_page(self) -> QWidget:
        page = QWidget()
        layout = QFormLayout(page)

        self.default_screen = QComboBox()
        self.default_screen.addItems(["AUTO", "CLOCK", "FFT", "STATUS"])
        self.default_screen.setCurrentText(str(self.settings.get("display.default_screen", "AUTO")))
        self.default_screen.currentTextChanged.connect(
            lambda value: self.setting_changed.emit("display.default_screen", value)
        )
        layout.addRow("Default Screen", self.default_screen)

        self.auto_connect = QCheckBox()
        self.auto_connect.setChecked(bool(self.settings.get("device.auto_connect", True)))
        self.auto_connect.toggled.connect(lambda value: self.setting_changed.emit("device.auto_connect", value))
        layout.addRow("Auto Connect", self.auto_connect)
        return page

    def update_state(self, state: AppState) -> None:
        self.esp_status.setText(state.esp32.status.value)
        self.pico_status.setText(state.pico.status.value)
        self.volume_label.setText(f"{state.audio.volume}%")
        self.media_label.setText("Playing" if state.audio.playing else "Paused")
        self.fft_label.setText(f"{state.audio.activity.value} / {state.audio.fft_fps} Hz")
        self.latency_label.setText("-" if state.esp32.latency_ms is None else f"{state.esp32.latency_ms} ms")
        self.volume_slider.setValue(state.audio.volume)

        while len(self.bars) < len(state.audio.bars):
            bar = QProgressBar()
            bar.setRange(0, 100)
            bar.setOrientation(Qt.Vertical)
            bar.setTextVisible(False)
            self.bars.append(bar)
        for bar, value in zip(self.bars, state.audio.bars):
            bar.setValue(value)

        self.device_detail.setText(
            f"ESP32-C3\n"
            f"Status: {state.esp32.status.value}\n"
            f"Connection Type: {state.esp32.connection_type}\n"
            f"Last Packet: {state.esp32.last_packet}\n\n"
            f"Pico 2\n"
            f"Status: {state.pico.status.value}\n"
            f"Connection Type: {state.pico.connection_type}\n"
            f"Last Packet: {state.pico.last_packet}"
        )
        self.log_view.setPlainText("\n".join(state.log_tail))
