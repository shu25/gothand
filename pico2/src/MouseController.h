#pragma once

#include <Arduino.h>

#include "Buttons.h"

class MouseController {
 public:
  void begin();
  void move(int dx, int dy, int wheel);
  void handleButton(const ButtonEvent& event);

 private:
  uint8_t buttonMask(MouseButtonAction action) const;
};
