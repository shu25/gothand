from __future__ import annotations

from collections.abc import Callable

import numpy as np


class AudioCapture:
    def __init__(self, sample_rate: int = 44100, chunk_size: int = 1024):
        self.sample_rate = sample_rate
        self.chunk_size = chunk_size
        self._recorder = None
        self._recorder_context = None

    def record_once(self) -> np.ndarray:
        try:
            import soundcard as sc

            if self._recorder is None:
                speaker = sc.default_speaker()
                microphone = sc.get_microphone(speaker.name, include_loopback=True)
                self._recorder_context = microphone.recorder(samplerate=self.sample_rate, channels=2)
                self._recorder = self._recorder_context.__enter__()
            return self._recorder.record(numframes=self.chunk_size)
        except Exception:
            self.close()
            return np.zeros((self.chunk_size, 1), dtype=np.float32)

    def stream(self, callback: Callable[[np.ndarray], None]) -> None:
        callback(self.record_once())

    def close(self) -> None:
        if self._recorder_context is not None:
            try:
                self._recorder_context.__exit__(None, None, None)
            except Exception:
                pass
        self._recorder = None
        self._recorder_context = None
