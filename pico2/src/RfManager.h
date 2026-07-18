#pragma once

#include <Arduino.h>

class RfManager {
 public:
  void begin();
  void update();
  bool connected() const;
  void sendMouseReport(int dx, int dy, int wheel, uint8_t buttons);

 private:
  bool connected_ = false;
};
