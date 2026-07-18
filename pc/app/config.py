from __future__ import annotations

import json
from copy import deepcopy
from pathlib import Path
from typing import Any


DEFAULT_SETTINGS: dict[str, Any] = {
    "device": {
        "esp32_name": "Godhand Controller",
        "auto_connect": True,
        "transport": "ble",
    },
    "audio": {
        "fft_enabled": True,
        "fft_bands": 16,
        "fft_update_rate": 30,
        "fft_sensitivity": 1.0,
        "fft_smoothing": 0.75,
        "silence_threshold": 0.02,
        "silence_detection_seconds": 5.0,
        "audio_detection_seconds": 1.0,
    },
    "display": {
        "default_screen": "AUTO",
        "volume_display_seconds": 2.5,
        "clock_format": "24h",
        "brightness": 255,
    },
    "controls": {
        "esp_buttons": {
            "1": "play_pause",
            "2": "previous",
            "3": "next",
            "4": "mode_change",
            "5": "mute",
        }
    },
    "app": {
        "start_with_windows": False,
        "start_minimized": False,
    },
}


def deep_merge(base: dict[str, Any], override: dict[str, Any]) -> dict[str, Any]:
    merged = deepcopy(base)
    for key, value in override.items():
        if isinstance(value, dict) and isinstance(merged.get(key), dict):
            merged[key] = deep_merge(merged[key], value)
        else:
            merged[key] = value
    return merged


class SettingsStore:
    def __init__(self, settings_path: Path, default_path: Path):
        self.settings_path = settings_path
        self.default_path = default_path
        self.data = self.load()

    def load(self) -> dict[str, Any]:
        self.default_path.parent.mkdir(parents=True, exist_ok=True)
        if not self.default_path.exists():
            self.default_path.write_text(json.dumps(DEFAULT_SETTINGS, indent=2), encoding="utf-8")

        defaults = json.loads(self.default_path.read_text(encoding="utf-8"))
        if not self.settings_path.exists():
            self.settings_path.parent.mkdir(parents=True, exist_ok=True)
            self.settings_path.write_text(json.dumps(defaults, indent=2), encoding="utf-8")
            return defaults

        try:
            loaded = json.loads(self.settings_path.read_text(encoding="utf-8"))
        except json.JSONDecodeError:
            backup = self.settings_path.with_suffix(".broken.json")
            self.settings_path.replace(backup)
            self.settings_path.write_text(json.dumps(defaults, indent=2), encoding="utf-8")
            return defaults

        return deep_merge(defaults, loaded)

    def save(self) -> None:
        self.settings_path.parent.mkdir(parents=True, exist_ok=True)
        self.settings_path.write_text(json.dumps(self.data, indent=2), encoding="utf-8")

    def get(self, dotted_key: str, default: Any = None) -> Any:
        current: Any = self.data
        for part in dotted_key.split("."):
            if not isinstance(current, dict) or part not in current:
                return default
            current = current[part]
        return current

    def set(self, dotted_key: str, value: Any) -> None:
        current = self.data
        parts = dotted_key.split(".")
        for part in parts[:-1]:
            current = current.setdefault(part, {})
        current[parts[-1]] = value
        self.save()
