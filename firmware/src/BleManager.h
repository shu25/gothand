#pragma once

#include "AppState.h"

class BleManager {
 public:
  void begin(AppState& state);
  void update(AppState& state);
  bool connected() const;

 private:
  bool connected_ = false;
};
