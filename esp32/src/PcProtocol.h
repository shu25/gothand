#pragma once

#include <Arduino.h>

#include "AppState.h"

class PcProtocol {
 public:
  void begin();
  void update(AppState& state);
  bool handleLine(AppState& state, const String& line);
};
