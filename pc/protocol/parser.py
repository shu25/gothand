from __future__ import annotations


def parse_line(line: str) -> tuple[str, str] | None:
    text = line.strip()
    if not text:
        return None
    if ":" not in text:
        return text.upper(), ""
    kind, payload = text.split(":", 1)
    return kind.upper(), payload.strip()
