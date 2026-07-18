#pragma once

#include <Arduino.h>

#include "AppState.h"

void beginDisplay();
void updateDisplay(AppState& state);
void setClockFromHms(AppState& state, const String& hms);
