#pragma once

#include "Encoder.h"
#include "PointingDevice.h"

class QuadratureTrackball : public PointingDevice {
 public:
  QuadratureTrackball(uint8_t xPinA, uint8_t xPinB, uint8_t yPinA, uint8_t yPinB);

  void begin() override;
  void update() override;
  int getDeltaX() override;
  int getDeltaY() override;

 private:
  Encoder x_;
  Encoder y_;
};
