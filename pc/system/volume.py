from __future__ import annotations

import ctypes
import sys


class VolumeController:
    def __init__(self):
        self._endpoint = None
        self._fallback_muted = False

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

    def get_mute(self) -> bool:
        endpoint = self._get_endpoint()
        if endpoint is not None:
            try:
                return bool(endpoint.GetMute())
            except Exception:
                pass
        return self._fallback_muted

    def change(self, delta: float) -> None:
        current = self.get()
        self.set((current if current is not None else 0.5) + delta)

    def mute(self) -> bool:
        endpoint = self._get_endpoint()
        if endpoint is not None:
            try:
                current = bool(endpoint.GetMute())
                muted = not current
                endpoint.SetMute(muted, None)
                self._fallback_muted = muted
                return muted
            except Exception:
                pass
        self._fallback_muted = not self._fallback_muted
        return self._fallback_muted

    def set_mute(self, muted: bool) -> None:
        self._fallback_muted = bool(muted)
        endpoint = self._get_endpoint()
        if endpoint is not None:
            try:
                endpoint.SetMute(bool(muted), None)
                return
            except Exception:
                pass
