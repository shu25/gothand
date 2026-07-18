from __future__ import annotations

from app.state import AudioActivity


class AudioActivityDetector:
    def __init__(self, silence_threshold: float, silence_seconds: float, active_seconds: float):
        self.silence_threshold = silence_threshold
        self.silence_seconds = silence_seconds
        self.active_seconds = active_seconds
        self.state = AudioActivity.SILENT
        self.candidate_since: float | None = None
        self.silent_since: float | None = None

    def update(self, level: float, now: float) -> AudioActivity:
        audible = level >= self.silence_threshold
        if self.state == AudioActivity.SILENT:
            if audible:
                self.state = AudioActivity.CANDIDATE
                self.candidate_since = now
        elif self.state == AudioActivity.CANDIDATE:
            if not audible:
                self.state = AudioActivity.SILENT
                self.candidate_since = None
            elif self.candidate_since is not None and now - self.candidate_since >= self.active_seconds:
                self.state = AudioActivity.ACTIVE
                self.silent_since = None
        elif self.state == AudioActivity.ACTIVE:
            if audible:
                self.silent_since = None
            else:
                if self.silent_since is None:
                    self.silent_since = now
                elif now - self.silent_since >= self.silence_seconds:
                    self.state = AudioActivity.SILENT
                    self.candidate_since = None
                    self.silent_since = None
        return self.state
