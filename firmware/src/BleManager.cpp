#include "BleManager.h"

#include <Arduino.h>
#include <NimBLEDevice.h>

#include "config.h"

namespace {
BleManager* activeManager = nullptr;
AppState* activeState = nullptr;

const __FlashStringHelper* displayModeName(DisplayMode mode) {
  switch (mode) {
    case DisplayMode::CLOCK:
      return F("clock");
    case DisplayMode::FFT:
      return F("fft");
    case DisplayMode::VOLUME:
      return F("volume");
    case DisplayMode::STATUS:
      return F("status");
  }
  return F("clock");
}

class CommandCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* characteristic, NimBLEConnInfo& connInfo) override {
    (void)connInfo;
    if (activeManager == nullptr || activeState == nullptr) {
      return;
    }

    std::string value = characteristic->getValue();
    if (value.empty()) {
      return;
    }

    activeManager->handleCommand(*activeState, String(value.c_str()));
  }
};

class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* server, NimBLEConnInfo& connInfo) override {
    (void)server;
    (void)connInfo;
    if (activeState != nullptr) {
      activeState->bluetoothConnected = true;
      activeState->pcConnected = true;
    }
    Serial.println(F("BLE connected"));
  }

  void onDisconnect(NimBLEServer* server, NimBLEConnInfo& connInfo, int reason) override {
    (void)server;
    (void)connInfo;
    (void)reason;
    if (activeState != nullptr) {
      activeState->bluetoothConnected = false;
      activeState->pcConnected = false;
    }
    NimBLEDevice::startAdvertising();
    Serial.println(F("BLE disconnected"));
  }
};
}

void BleManager::begin(AppState& state) {
  activeManager = this;
  activeState = &state;
  protocol_.begin();

  NimBLEDevice::init(BLE_DEVICE_NAME);
  server_ = NimBLEDevice::createServer();
  server_->setCallbacks(new ServerCallbacks());

  NimBLEService* service = server_->createService(SERVICE_UUID);
  NimBLECharacteristic* commandCharacteristic = service->createCharacteristic(
      CMD_UUID,
      NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
  stateCharacteristic_ = service->createCharacteristic(
      STATE_UUID,
      NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);

  commandCharacteristic->setCallbacks(new CommandCallbacks());
  server_->start();

  NimBLEAdvertising* advertising = NimBLEDevice::getAdvertising();
  advertising->setName(BLE_DEVICE_NAME);
  advertising->addServiceUUID(SERVICE_UUID);
  advertising->enableScanResponse(true);
  advertising->setMinInterval(0x20);
  advertising->setMaxInterval(0x40);
  NimBLEDevice::startAdvertising();

  connected_ = false;
  state.bluetoothConnected = false;
  state.pcConnected = false;
  Serial.println(F("BLE advertising started"));
}

void BleManager::update(AppState& state) {
  connected_ = server_ != nullptr && server_->getConnectedCount() > 0;
  state.bluetoothConnected = connected_;
  state.pcConnected = connected_;

  const String event = state.consumeEvent();
  if (event.length() > 0) {
    notify(event);
  }

  const uint32_t now = millis();
  if (now - lastTelemetry_ >= TELEMETRY_INTERVAL_MS) {
    lastTelemetry_ = now;
    notifyState(state);
  }
}

bool BleManager::connected() const {
  return connected_;
}

void BleManager::handleCommand(AppState& state, const String& command) {
  if (protocol_.handleLine(state, command)) {
    notifyState(state);
  } else {
    Serial.print(F("Unhandled BLE command: "));
    Serial.println(command);
  }
}

void BleManager::notify(const String& message) {
  if (stateCharacteristic_ == nullptr || !connected_) {
    return;
  }

  stateCharacteristic_->setValue(message.c_str());
  stateCharacteristic_->notify();
}

void BleManager::notifyState(const AppState& state) {
  if (stateCharacteristic_ == nullptr || !connected_) {
    return;
  }

  String telemetry = F("VOL:");
  telemetry += String(state.volume);
  telemetry += F("\nSTATE:");
  telemetry += state.audioPlaying ? F("PLAY") : F("PAUSE");
  telemetry += F("\nMODE:");
  telemetry += state.controlMode;
  telemetry += F("\nDISP:");
  telemetry += displayModeName(state.displayMode);

  notify(telemetry);
}
