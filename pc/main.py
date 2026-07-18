from __future__ import annotations

import sys
from pathlib import Path

from PySide6.QtWidgets import QApplication

from app.application import TanaControllerApp
from app.config import SettingsStore
from system.logger import configure_logging


def main() -> int:
    root = Path(__file__).resolve().parent
    configure_logging(root / "logs" / "app.log")

    qt_app = QApplication(sys.argv)
    qt_app.setApplicationName("Tana Controller")

    settings = SettingsStore(root / "config" / "settings.json", root / "config" / "default.json")
    controller = TanaControllerApp(settings)
    controller.show()

    return qt_app.exec()


if __name__ == "__main__":
    raise SystemExit(main())
