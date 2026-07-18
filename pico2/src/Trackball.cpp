#include "Trackball.h"

QuadratureTrackball::QuadratureTrackball(
    uint8_t xPinA,
    uint8_t xPinB,
    uint8_t yPinA,
    uint8_t yPinB)
    : x_(xPinA, xPinB), y_(yPinA, yPinB) {}

void QuadratureTrackball::begin() {
  x_.begin();
  y_.begin();
}

void QuadratureTrackball::update() {
  x_.update();
  y_.update();
}

int QuadratureTrackball::getDeltaX() {
  return x_.readSteps();
}

int QuadratureTrackball::getDeltaY() {
  return y_.readSteps();
}
