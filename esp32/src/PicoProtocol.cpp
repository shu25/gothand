#include "PicoProtocol.h"

#include "config.h"

void PicoProtocol::begin(Stream& serial) {
  serial_ = &serial;
  rx_ = "";
  lastPacketMillis_ = 0;
}

void PicoProtocol::update(AppState& state) {
  if (serial_ == nullptr) {
    state.picoConnected = false;
    return;
  }

  while (serial_->available() > 0) {
    const char c = static_cast<char>(serial_->read());
    if (c == '\n') {
      handleLine(state, rx_);
      rx_ = "";
    } else if (c != '\r') {
      rx_ += c;
      if (rx_.length() > 80) {
        rx_ = "";
      }
    }
  }

  state.picoConnected =
      lastPacketMillis_ > 0 && millis() - lastPacketMillis_ < PICO_LINK_TIMEOUT_MS;
}

void PicoProtocol::sendVolume(uint8_t volume) {
  if (serial_ == nullptr) {
    return;
  }

  serial_->print(F("VOL:"));
  serial_->println(volume);
}

void PicoProtocol::handleLine(AppState& state, const String& line) {
  String text = line;
  text.trim();
  if (text.length() == 0) {
    return;
  }

  lastPacketMillis_ = millis();
  state.picoConnected = true;

  Serial.print(F("PICO> "));
  Serial.println(text);
}
