from __future__ import annotations


COMMANDS = {
    "play_pause": "BTN:PLAY",
    "next": "BTN:NEXT",
    "previous": "BTN:PREV",
    "mute": "BTN:MUTE",
    "mode_change": "BTN:MODE",
}


def encode_command(action: str) -> str:
    return COMMANDS.get(action, f"BTN:{action.upper()}")


def encode_fft(values: list[int]) -> str:
    return "FFT:" + ",".join(str(max(0, min(100, int(value)))) for value in values)


def encode_volume(volume: int) -> str:
    return f"VOL:{max(0, min(100, int(volume)))}"
