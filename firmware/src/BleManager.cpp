#include "BleManager.h"

#include <Arduino.h>

void BleManager::begin(AppState& state) {
  connected_ = false;
  state.bluetoothConnected = false;
  Serial.println(F("BleManager stub ready"));
}

void BleManager::update(AppState& state) {
  state.bluetoothConnected = connected_;
}

bool BleManager::connected() const {
  return connected_;
}
