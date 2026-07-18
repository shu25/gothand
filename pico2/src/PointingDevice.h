#pragma once

class PointingDevice {
 public:
  virtual ~PointingDevice() = default;
  virtual void begin() = 0;
  virtual void update() = 0;
  virtual int getDeltaX() = 0;
  virtual int getDeltaY() = 0;
};
