from __future__ import annotations

import ctypes
import sys


class VolumeController:
    def __init__(self):
        self._endpoint = None

    def _get_endpoint(self):
        if sys.platform != "win32":
            return None
        if self._endpoint is not None:
            return self._endpoint
        try:
            from pycaw.pycaw import AudioUtilities

            speakers = AudioUtilities.GetSpeakers()
            self._endpoint = speakers.EndpointVolume
        except Exception:
            self._endpoint = None
        return self._endpoint

    def get(self) -> float | None:
        endpoint = self._get_endpoint()
        if endpoint is None:
            return None
        try:
            return float(endpoint.GetMasterVolumeLevelScalar())
        except Exception:
            return None

    def set(self, value: float) -> None:
        volume = max(0.0, min(1.0, value))
        endpoint = self._get_endpoint()
        if endpoint is not None:
            try:
                endpoint.SetMasterVolumeLevelScalar(volume, None)
                return
            except Exception:
                pass
        if sys.platform == "win32":
            winmm = ctypes.windll.winmm
            raw = int(volume * 0xFFFF) & 0xFFFF
            winmm.waveOutSetVolume(0xFFFFFFFF, (raw << 16) | raw)

    def change(self, delta: float) -> None:
        current = self.get()
        self.set((current if current is not None else 0.5) + delta)

    def mute(self) -> None:
        endpoint = self._get_endpoint()
        if endpoint is not None:
            try:
                current = bool(endpoint.GetMute())
                endpoint.SetMute(not current, None)
                return
            except Exception:
                pass
