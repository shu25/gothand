#include "EspProtocol.h"

void EspProtocol::begin(Stream& serial) {
  serial_ = &serial;
}

void EspProtocol::update() {
  if (serial_ == nullptr) {
    return;
  }

  while (serial_->available() > 0) {
    const char c = static_cast<char>(serial_->read());
    if (c == '\n') {
      handleLine(rx_);
      rx_ = "";
    } else if (c != '\r') {
      rx_ += c;
      if (rx_.length() > 80) {
        rx_ = "";
      }
    }
  }
}

void EspProtocol::sendMouseDelta(int dx, int dy) {
  if (serial_ == nullptr || (dx == 0 && dy == 0)) {
    return;
  }
  serial_->print(F("MOUSE:"));
  serial_->print(dx);
  serial_->print(F(","));
  serial_->println(dy);
}

void EspProtocol::sendScroll(int wheel) {
  if (serial_ == nullptr || wheel == 0) {
    return;
  }
  serial_->print(F("SCROLL:"));
  serial_->println(wheel);
}

void EspProtocol::sendButton(uint8_t index, bool pressed) {
  if (serial_ == nullptr) {
    return;
  }
  serial_->print(F("BTN:"));
  serial_->print(index + 1);
  serial_->println(pressed ? F(":DOWN") : F(":UP"));
}

void EspProtocol::sendStatus() {
  if (serial_ != nullptr) {
    serial_->println(F("PICO:READY"));
  }
}

void EspProtocol::handleLine(const String& line) {
  String text = line;
  text.trim();
  if (text.length() == 0) {
    return;
  }

  Serial.print(F("ESP> "));
  Serial.println(text);
}
