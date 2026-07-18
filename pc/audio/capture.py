from __future__ import annotations

from collections.abc import Callable

import numpy as np


class AudioCapture:
    def __init__(self, sample_rate: int = 44100, chunk_size: int = 1024):
        self.sample_rate = sample_rate
        self.chunk_size = chunk_size

    def record_once(self) -> np.ndarray:
        try:
            import soundcard as sc

            speaker = sc.default_speaker()
            microphone = sc.get_microphone(speaker.name, include_loopback=True)
            with microphone.recorder(samplerate=self.sample_rate, channels=2) as recorder:
                return recorder.record(numframes=self.chunk_size)
        except Exception:
            return np.zeros((self.chunk_size, 1), dtype=np.float32)

    def stream(self, callback: Callable[[np.ndarray], None]) -> None:
        callback(self.record_once())
