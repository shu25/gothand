#include "PcProtocol.h"

#include "DisplayUi.h"
#include "config.h"

void PcProtocol::begin() {
  Serial.println(F("PcProtocol stub ready"));
}

void PcProtocol::update(AppState& state) {
  state.pcConnected = false;
}

bool PcProtocol::handleLine(AppState& state, const String& line) {
  String text = line;
  text.trim();

  if (text.startsWith(F("VOL:"))) {
    state.setVolume(text.substring(4).toInt());
    return true;
  }

  if (text.startsWith(F("TIME:"))) {
    setClockFromHms(state, text.substring(5));
    return true;
  }

  if (text.startsWith(F("FFT:"))) {
    uint8_t band = 0;
    int start = 4;
    while (band < state.fftBandCount && start >= 0 && start < text.length()) {
      const int comma = text.indexOf(',', start);
      const String value = comma < 0 ? text.substring(start) : text.substring(start, comma);
      state.fftBars[band++] = constrain(value.toInt(), 0, 100);
      start = comma < 0 ? -1 : comma + 1;
    }
    while (band < state.fftBandCount) {
      state.fftBars[band++] = 0;
    }
    state.fftDataValid = true;
    state.displayMode = DisplayMode::FFT;
    state.previousDisplayMode = DisplayMode::FFT;
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

  return false;
}
