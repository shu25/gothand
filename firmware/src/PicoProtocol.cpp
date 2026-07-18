#include "PicoProtocol.h"

void PicoProtocol::begin(Stream& serial) {
  serial_ = &serial;
}

void PicoProtocol::update(AppState& state) {
  state.picoConnected = serial_ != nullptr;
}

void PicoProtocol::sendVolume(uint8_t volume) {
  if (serial_ == nullptr) {
    return;
  }

  serial_->print(F("VOL:"));
  serial_->println(volume);
}
