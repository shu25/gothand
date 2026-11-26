#include "PcProtocol.h"

#include "DisplayUi.h"
#include "config.h"

namespace {
uint8_t decodePackedValue(char c) {
  if (c >= 'A' && c <= 'Z') {
    return map(c - 'A', 0, 63, 0, 100);
  }
  if (c >= 'a' && c <= 'z') {
    return map(c - 'a' + 26, 0, 63, 0, 100);
  }
  if (c >= '0' && c <= '9') {
    return map(c - '0' + 52, 0, 63, 0, 100);
  }
  if (c == '+') {
    return map(62, 0, 63, 0, 100);
  }
  if (c == '/') {
    return 100;
  }
  return 0;
}

DisplayMode parseDisplayMode(String value, DisplayMode fallback) {
  value.trim();
  value.toLowerCase();
  if (value == F("clock") || value == F("auto")) {
    return DisplayMode::CLOCK;
  }
  if (value == F("disc") || value == F("disk") || value == F("record") ||
      value == F("fft") || value == F("wave") || value == F("status")) {
    return DisplayMode::DISC;
  }
  return fallback;
}
}

void PcProtocol::begin() {
  Serial.println(F("PcProtocol ready"));
}

void PcProtocol::update(AppState& state) {
  (void)state;
}

bool PcProtocol::handleLine(AppState& state, const String& line) {
  String text = line;
  text.trim();

  if (text.startsWith(F("EVENT:"))) {
    return true;
  }

  if (text == F("PLAY_PAUSE")) {
    return true;
  }

  if (text.startsWith(F("VOL:"))) {
    state.setVolume(text.substring(4).toInt());
    state.muted = false;
    return true;
  }

  if (text == F("MUTE")) {
    state.muted = !state.muted;
    state.showVolumeTemporarily();
    return true;
  }

  if (text.startsWith(F("MUTE:"))) {
    String value = text.substring(5);
    value.trim();
    value.toLowerCase();
    state.muted = value == F("1") || value == F("on") || value == F("true");
    state.showVolumeTemporarily();
    return true;
  }

  if (text.startsWith(F("TIME:"))) {
    setClockFromHms(state, text.substring(5));
    return true;
  }

  if (text.startsWith(F("MODE:"))) {
    const String value = text.substring(5);
    if (value.length() > 0) {
      state.controlMode = value.charAt(0);
    }
    return true;
  }

  if (text.startsWith(F("DISP:"))) {
    const DisplayMode next = parseDisplayMode(text.substring(5), state.displayMode);
    state.displayMode = next;
    if (next != DisplayMode::VOLUME) {
      state.previousDisplayMode = next;
    }
    return true;
  }

  if (text.startsWith(F("CLOCK:"))) {
    String value = text.substring(6);
    value.trim();
    value.toLowerCase();
    state.clockEnabled = value == F("on") || value == F("1") || value == F("true");
    return true;
  }

  if (text.startsWith(F("FFT:"))) {
    uint8_t band = 0;
    int start = 4;
    while (band < MAX_FFT_BANDS && start >= 0 && start < text.length()) {
      const int comma = text.indexOf(',', start);
      const String value = comma < 0 ? text.substring(start) : text.substring(start, comma);
      state.fftBars[band++] = constrain(value.toInt(), 0, 100);
      start = comma < 0 ? -1 : comma + 1;
    }
    const uint8_t parsedBands = band;
    while (band < MAX_FFT_BANDS) {
      state.fftBars[band++] = 0;
    }
    state.fftBandCount = max<uint8_t>(1, parsedBands);
    state.fftDataValid = true;
    state.waveMode = false;
    state.displayMode = DisplayMode::DISC;
    state.previousDisplayMode = DisplayMode::DISC;
    return true;
  }

  if (text.startsWith(F("B:"))) {
    const int payloadStart = text.indexOf(':', 2);
    if (payloadStart < 0) {
      return false;
    }
    const int payloadEnd = text.indexOf(':', payloadStart + 1);
    const char visualMode = payloadStart > 2 ? text.charAt(2) : 'F';
    const String payload = payloadEnd < 0
        ? text.substring(payloadStart + 1)
        : text.substring(payloadStart + 1, payloadEnd);
    const uint8_t count = min<uint8_t>(MAX_FFT_BANDS, payload.length());
    for (uint8_t i = 0; i < count; ++i) {
      state.fftBars[i] = decodePackedValue(payload.charAt(i));
    }
    for (uint8_t i = count; i < MAX_FFT_BANDS; ++i) {
      state.fftBars[i] = 0;
    }
    state.fftBandCount = max<uint8_t>(1, count);
    state.fftDataValid = true;
    state.waveMode = visualMode == 'W' || visualMode == 'w';
    state.lastAudioActivity = millis();
    state.displayMode = DisplayMode::DISC;
    state.previousDisplayMode = state.displayMode;
    return true;
  }

  if (text == F("PLAY")) {
    state.audioPlaying = true;
    return true;
  }

  if (text == F("PAUSE")) {
    state.audioPlaying = false;
    return true;
  }

  if (text.startsWith(F("STATE:"))) {
    String value = text.substring(6);
    value.trim();
    value.toUpperCase();
    if (value == F("PLAY")) {
      state.audioPlaying = true;
      return true;
    }
    if (value == F("PAUSE")) {
      state.audioPlaying = false;
      return true;
    }
  }

  return false;
}
