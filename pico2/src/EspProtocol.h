#pragma once

#include <Arduino.h>

class EspProtocol {
 public:
  void begin(Stream& serial);
  void update();
  void sendMouseDelta(int dx, int dy);
  void sendScroll(int wheel);
  void sendButton(uint8_t index, bool pressed);
  void sendStatus();

 private:
  Stream* serial_ = nullptr;
  String rx_;
  void handleLine(const String& line);
};
