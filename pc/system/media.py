from __future__ import annotations

import ctypes
import sys


class MediaController:
    def play_pause(self) -> None:
        self._send_windows_key(0xB3)

    def next(self) -> None:
        self._send_windows_key(0xB0)

    def previous(self) -> None:
        self._send_windows_key(0xB1)

    def _send_windows_key(self, key_code: int) -> None:
        if sys.platform != "win32":
            return
        keyeventf_keyup = 0x0002
        ctypes.windll.user32.keybd_event(key_code, 0, 0, 0)
        ctypes.windll.user32.keybd_event(key_code, 0, keyeventf_keyup, 0)
