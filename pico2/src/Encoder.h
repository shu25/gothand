#pragma once

#include <Arduino.h>

class Encoder {
 public:
  Encoder(uint8_t pinA, uint8_t pinB);

  void begin();
  void update();
  int readSteps();

 private:
  uint8_t pinA_;
  uint8_t pinB_;
  uint8_t lastState_ = 0;
  int8_t transitionSum_ = 0;
  int steps_ = 0;
};
