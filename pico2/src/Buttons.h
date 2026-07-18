#pragma once

#include <Arduino.h>

enum class MouseButtonAction : uint8_t {
  LEFT,
  RIGHT,
  MIDDLE,
  BACK,
  FORWARD,
  MACRO_1,
  MACRO_2,
  MACRO_3,
  MODE,
  CUSTOM
};

struct ButtonEvent {
  uint8_t index = 0;
  MouseButtonAction action = MouseButtonAction::CUSTOM;
  bool pressed = false;
  bool valid = false;
};

class Buttons {
 public:
  void begin();
  ButtonEvent update();

 private:
  bool stable_[10] = {HIGH};
  bool lastReading_[10] = {HIGH};
  uint32_t lastChange_[10] = {0};
};
