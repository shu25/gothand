#include "AppState.h"

#include "config.h"

void AppState::setVolume(int newVolume) {
  volume = constrain(newVolume, 0, 100);
  showVolumeTemporarily();
}

void AppState::changeVolume(int delta) {
  setVolume(volume + delta);
}

void AppState::showVolumeTemporarily() {
  const uint32_t now = millis();
  if (displayMode != DisplayMode::VOLUME) {
    previousDisplayMode = displayMode;
  }
  displayMode = DisplayMode::VOLUME;
  lastVolumeChange = now;
  lastInputActivity = now;
}

void AppState::togglePlayback() {
  audioPlaying = !audioPlaying;
  lastInputActivity = millis();
}

void AppState::applyButtonAction(ButtonAction action) {
  lastInputActivity = millis();

  switch (action) {
    case ButtonAction::PLAY_PAUSE:
      togglePlayback();
      break;
    case ButtonAction::PREVIOUS_TRACK:
      Serial.println(F("ACTION:PREVIOUS_TRACK"));
      break;
    case ButtonAction::NEXT_TRACK:
      Serial.println(F("ACTION:NEXT_TRACK"));
      break;
    case ButtonAction::MODE_CHANGE:
      cycleDisplayMode();
      break;
    case ButtonAction::MUTE:
      muted = !muted;
      break;
    case ButtonAction::CUSTOM:
      Serial.println(F("ACTION:CUSTOM"));
      break;
  }
}

void AppState::cycleDisplayMode() {
  switch (displayMode) {
    case DisplayMode::CLOCK:
      displayMode = DisplayMode::FFT;
      break;
    case DisplayMode::FFT:
      displayMode = DisplayMode::STATUS;
      break;
    case DisplayMode::STATUS:
    case DisplayMode::VOLUME:
      displayMode = DisplayMode::CLOCK;
      break;
  }

  if (displayMode != DisplayMode::VOLUME) {
    previousDisplayMode = displayMode;
  }
}

void AppState::setClock(uint8_t hours, uint8_t minutes, uint8_t seconds) {
  if (hours > 23 || minutes > 59 || seconds > 59) {
    return;
  }

  clockSecondsAtSync = (hours * 3600UL) + (minutes * 60UL) + seconds;
  clockSyncMillis = millis();
  clockSynced = true;
}

uint32_t AppState::currentClockSeconds(uint32_t now) const {
  if (!clockSynced) {
    return (now / 1000UL) % 86400UL;
  }

  return (clockSecondsAtSync + ((now - clockSyncMillis) / 1000UL)) % 86400UL;
}

void AppState::update(uint32_t now) {
  if (displayMode == DisplayMode::VOLUME &&
      now - lastVolumeChange >= VOLUME_SCREEN_TIMEOUT_MS) {
    displayMode = previousDisplayMode;
  }
}
