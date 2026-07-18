from __future__ import annotations

import numpy as np


class FFTAnalyzer:
    def __init__(self, bands: int = 16, smoothing: float = 0.75, sensitivity: float = 1.0):
        self.bands = bands
        self.smoothing = smoothing
        self.sensitivity = sensitivity
        self._smooth = np.zeros(bands, dtype=np.float32)

    def set_bands(self, bands: int) -> None:
        self.bands = bands
        self._smooth = np.zeros(bands, dtype=np.float32)

    def analyze(self, samples: np.ndarray, sample_rate: int) -> list[int]:
        if samples.size == 0:
            return [0] * self.bands
        mono = samples.astype(np.float32)
        if mono.ndim > 1:
            mono = mono.mean(axis=1)
        mono = mono - mono.mean()
        windowed = mono * np.hanning(len(mono))
        spectrum = np.abs(np.fft.rfft(windowed))
        freqs = np.fft.rfftfreq(len(mono), 1 / sample_rate)

        edges = np.geomspace(50, min(sample_rate / 2, 12000), self.bands + 1)
        values = []
        for low, high in zip(edges[:-1], edges[1:]):
            mask = (freqs >= low) & (freqs < high)
            level = float(np.mean(spectrum[mask])) if np.any(mask) else 0.0
            values.append(np.log1p(level) * self.sensitivity)

        raw = np.asarray(values, dtype=np.float32)
        denom = float(raw.max()) or 1.0
        normalized = np.clip((raw / denom) * 100, 0, 100)
        self._smooth = (self.smoothing * self._smooth) + ((1.0 - self.smoothing) * normalized)
        return self._smooth.astype(int).tolist()
