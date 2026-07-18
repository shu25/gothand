#pragma once

#include <Arduino.h>

#include "AppState.h"

class PicoProtocol {
 public:
  void begin(Stream& serial);
  void update(AppState& state);
  void sendVolume(uint8_t volume);

 private:
  Stream* serial_ = nullptr;
  String rx_;
  uint32_t lastPacketMillis_ = 0;

  void handleLine(AppState& state, const String& line);
};
