#pragma once

#include "AppState.h"
#include "PcProtocol.h"

class NimBLECharacteristic;
class NimBLEServer;

class BleManager {
 public:
  void begin(AppState& state);
  void update(AppState& state);
  bool connected() const;
  void handleCommand(AppState& state, const String& command);

 private:
  bool connected_ = false;
  uint32_t lastTelemetry_ = 0;
  NimBLEServer* server_ = nullptr;
  NimBLECharacteristic* stateCharacteristic_ = nullptr;
  PcProtocol protocol_;

  void notify(const String& message);
  void notifyState(const AppState& state);
};
