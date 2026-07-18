#include "Buttons.h"

#include "config.h"

namespace {
constexpr MouseButtonAction ACTIONS[MOUSE_BUTTON_COUNT] = {
    MouseButtonAction::LEFT,
    MouseButtonAction::RIGHT,
    MouseButtonAction::MIDDLE,
    MouseButtonAction::BACK,
    MouseButtonAction::FORWARD,
    MouseButtonAction::MACRO_1,
    MouseButtonAction::MACRO_2,
    MouseButtonAction::MACRO_3,
    MouseButtonAction::MODE,
    MouseButtonAction::CUSTOM,
};
}

void Buttons::begin() {
  for (uint8_t i = 0; i < MOUSE_BUTTON_COUNT; ++i) {
    pinMode(MOUSE_BUTTON_PINS[i], INPUT_PULLUP);
    stable_[i] = digitalRead(MOUSE_BUTTON_PINS[i]);
    lastReading_[i] = stable_[i];
  }
}

ButtonEvent Buttons::update() {
  const uint32_t now = millis();
  for (uint8_t i = 0; i < MOUSE_BUTTON_COUNT; ++i) {
    const bool reading = digitalRead(MOUSE_BUTTON_PINS[i]);
    if (reading != lastReading_[i]) {
      lastReading_[i] = reading;
      lastChange_[i] = now;
    }

    if (now - lastChange_[i] < INPUT_DEBOUNCE_MS || reading == stable_[i]) {
      continue;
    }

    stable_[i] = reading;
    return ButtonEvent{
        i,
        ACTIONS[i],
        stable_[i] == LOW,
        true,
    };
  }

  return ButtonEvent{};
}
