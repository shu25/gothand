#pragma once

#include <Arduino.h>

enum class DisplayMode : uint8_t {
  CLOCK,
  FFT,
  WAVE,
  DISC,
  VOLUME,
  STATUS
};

enum class ButtonAction : uint8_t {
  PLAY_PAUSE,
  PREVIOUS_TRACK,
  NEXT_TRACK,
  MODE_CHANGE,
  MUTE,
  CUSTOM
};

enum class FeedbackIcon : uint8_t {
  NONE,
  PLAYBACK,
  PREVIOUS,
  NEXT,
  MODE,
  MUTE,
  VOLUME_UP,
  VOLUME_DOWN
};

struct AppState {
  DisplayMode displayMode = DisplayMode::CLOCK;
  DisplayMode previousDisplayMode = DisplayMode::CLOCK;

  int volume = 50;
  bool muted = false;
  bool bluetoothConnected = false;
  bool pcConnected = false;
  bool picoConnected = false;
  bool audioPlaying = false;
  bool clockEnabled = true;
  char controlMode = 'A';

  uint32_t lastVolumeChange = 0;
  uint32_t lastAudioActivity = 0;
  uint32_t lastDisplayUpdate = 0;
  uint32_t lastInputActivity = 0;
  uint32_t feedbackUntil = 0;
  FeedbackIcon feedbackIcon = FeedbackIcon::NONE;

  uint8_t fftBars[32] = {0};
  uint8_t fftBandCount = 16;
  bool fftDataValid = false;
  bool waveMode = false;

  uint32_t clockSyncMillis = 0;
  uint32_t clockSecondsAtSync = 0;
  bool clockSynced = false;
  String pendingEvent;

  void setVolume(int newVolume);
  void changeVolume(int delta);
  void queueEvent(const String& event);
  String consumeEvent();
  void showFeedback(FeedbackIcon icon);
  void showVolumeTemporarily();
  void togglePlayback();
  void applyButtonAction(ButtonAction action);
  void cycleDisplayMode();
  void setClock(uint8_t hours, uint8_t minutes, uint8_t seconds);
  uint32_t currentClockSeconds(uint32_t now) const;
  void update(uint32_t now);
};
