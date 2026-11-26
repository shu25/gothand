#include "AppState.h"

#include "config.h"

void AppState::setVolume(int newVolume) {
  volume = constrain(newVolume, 0, 100);
  showVolumeTemporarily();
}

void AppState::changeVolume(int delta) {
  if (delta > 0) {
    showFeedback(FeedbackIcon::VOLUME_UP);
  } else if (delta < 0) {
    showFeedback(FeedbackIcon::VOLUME_DOWN);
  }
  setVolume(volume + delta);
}

void AppState::queueEvent(const String& event) {
  pendingEvent = event;
}

String AppState::consumeEvent() {
  String event = pendingEvent;
  pendingEvent = "";
  return event;
}

void AppState::showFeedback(FeedbackIcon icon) {
  feedbackIcon = icon;
  feedbackUntil = millis() + 1000UL;
}

void AppState::showVolumeTemporarily() {
  const uint32_t now = millis();
  if (displayMode == DisplayMode::DISC || displayMode == DisplayMode::FFT || displayMode == DisplayMode::WAVE) {
    lastVolumeChange = now;
    lastInputActivity = now;
    return;
  }

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
      showFeedback(FeedbackIcon::PLAYBACK);
      break;
    case ButtonAction::PREVIOUS_TRACK:
      showFeedback(FeedbackIcon::PREVIOUS);
      Serial.println(F("ACTION:PREVIOUS_TRACK"));
      break;
    case ButtonAction::NEXT_TRACK:
      showFeedback(FeedbackIcon::NEXT);
      Serial.println(F("ACTION:NEXT_TRACK"));
      break;
    case ButtonAction::MODE_CHANGE:
      showFeedback(FeedbackIcon::MODE);
      cycleDisplayMode();
      queueEvent(F("EVENT:BTN4"));
      break;
    case ButtonAction::MUTE:
      muted = !muted;
      showFeedback(FeedbackIcon::MUTE);
      break;
    case ButtonAction::CUSTOM:
      showFeedback(FeedbackIcon::MODE);
      Serial.println(F("ACTION:CUSTOM"));
      break;
  }
}

void AppState::cycleDisplayMode() {
  switch (displayMode) {
    case DisplayMode::CLOCK:
      displayMode = DisplayMode::DISC;
      break;
    case DisplayMode::FFT:
    case DisplayMode::WAVE:
    case DisplayMode::DISC:
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
