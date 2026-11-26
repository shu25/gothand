#include "Inputs.h"

#include <Arduino.h>

#include "config.h"

namespace {
struct DebouncedInput {
  bool stable = HIGH;
  bool lastReading = HIGH;
  uint32_t lastChange = 0;
};

DebouncedInput buttons[BUTTON_COUNT];
DebouncedInput encoderButton;

volatile int encoderSteps = 0;
volatile uint8_t lastEncoderState = 0;
volatile int8_t encoderTransitionSum = 0;

void IRAM_ATTR handleEncoderChange() {
  static const int8_t transitionTable[16] = {
      0, -1, 1, 0,
      1, 0, 0, -1,
      -1, 0, 0, 1,
      0, 1, -1, 0};

  const uint8_t state = (digitalRead(ENCODER_A_PIN) << 1) | digitalRead(ENCODER_B_PIN);
  const uint8_t transition = (lastEncoderState << 2) | state;
  const int8_t movement = transitionTable[transition];
  lastEncoderState = state;

  if (movement == 0) {
    return;
  }

  encoderTransitionSum += movement;
  if (encoderTransitionSum >= 4) {
    ++encoderSteps;
    encoderTransitionSum = 0;
  } else if (encoderTransitionSum <= -4) {
    --encoderSteps;
    encoderTransitionSum = 0;
  }
}

bool fell(DebouncedInput& input, uint8_t pin, uint32_t now) {
  const bool reading = digitalRead(pin);

  if (reading != input.lastReading) {
    input.lastReading = reading;
    input.lastChange = now;
  }

  if (now - input.lastChange < INPUT_DEBOUNCE_MS || reading == input.stable) {
    return false;
  }

  const bool previous = input.stable;
  input.stable = reading;
  return previous == HIGH && input.stable == LOW;
}
}

void beginInputs() {
  for (uint8_t i = 0; i < BUTTON_COUNT; ++i) {
    pinMode(BUTTON_PINS[i], INPUT_PULLUP);
    buttons[i].stable = digitalRead(BUTTON_PINS[i]);
    buttons[i].lastReading = buttons[i].stable;
  }

  pinMode(ENCODER_A_PIN, INPUT_PULLUP);
  pinMode(ENCODER_B_PIN, INPUT_PULLUP);
  pinMode(ENCODER_SW_PIN, INPUT_PULLUP);

  encoderButton.stable = digitalRead(ENCODER_SW_PIN);
  encoderButton.lastReading = encoderButton.stable;
  lastEncoderState = (digitalRead(ENCODER_A_PIN) << 1) | digitalRead(ENCODER_B_PIN);

  attachInterrupt(digitalPinToInterrupt(ENCODER_A_PIN), handleEncoderChange, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_B_PIN), handleEncoderChange, CHANGE);
}

void updateInputs(AppState& state) {
  const uint32_t now = millis();

  for (uint8_t i = 0; i < BUTTON_COUNT; ++i) {
    if (fell(buttons[i], BUTTON_PINS[i], now)) {
      state.queueEvent("EVENT:BTN" + String(i + 1));
      state.applyButtonAction(BUTTON_ACTIONS[i]);
    }
  }

  int steps = 0;
  noInterrupts();
  steps = encoderSteps;
  encoderSteps = 0;
  interrupts();

  if (steps != 0) {
    state.changeVolume(steps * ENCODER_VOLUME_STEP);
    state.queueEvent("EVENT:ENC:" + String(steps > 0 ? "+" : "-") + String(abs(steps)) + ":" + String(state.volume));
    Serial.print(F("VOL:"));
    Serial.println(state.volume);
  }

  if (fell(encoderButton, ENCODER_SW_PIN, now)) {
    state.applyButtonAction(ButtonAction::PLAY_PAUSE);
    state.queueEvent(state.audioPlaying ? F("EVENT:PLAY") : F("EVENT:PAUSE"));
    Serial.println(state.audioPlaying ? F("PLAY") : F("PAUSE"));
  }
}
