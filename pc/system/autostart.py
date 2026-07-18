from __future__ import annotations


class AutostartManager:
    def is_enabled(self) -> bool:
        return False

    def set_enabled(self, enabled: bool) -> None:
        del enabled
