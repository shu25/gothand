#include "Encoder.h"

Encoder::Encoder(uint8_t pinA, uint8_t pinB) : pinA_(pinA), pinB_(pinB) {}

void Encoder::begin() {
  pinMode(pinA_, INPUT_PULLUP);
  pinMode(pinB_, INPUT_PULLUP);
  lastState_ = (digitalRead(pinA_) << 1) | digitalRead(pinB_);
}

void Encoder::update() {
  static const int8_t transitionTable[16] = {
      0, -1, 1, 0,
      1, 0, 0, -1,
      -1, 0, 0, 1,
      0, 1, -1, 0};

  const uint8_t state = (digitalRead(pinA_) << 1) | digitalRead(pinB_);
  const uint8_t transition = (lastState_ << 2) | state;
  lastState_ = state;

  const int8_t movement = transitionTable[transition];
  if (movement == 0) {
    return;
  }

  transitionSum_ += movement;
  if (transitionSum_ >= 4) {
    ++steps_;
    transitionSum_ = 0;
  } else if (transitionSum_ <= -4) {
    --steps_;
    transitionSum_ = 0;
  }
}

int Encoder::readSteps() {
  const int steps = steps_;
  steps_ = 0;
  return steps;
}
