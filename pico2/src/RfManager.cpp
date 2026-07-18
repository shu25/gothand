#include "RfManager.h"

void RfManager::begin() {
  connected_ = false;
  Serial.println(F("RF manager stub ready"));
}

void RfManager::update() {}

bool RfManager::connected() const {
  return connected_;
}

void RfManager::sendMouseReport(int dx, int dy, int wheel, uint8_t buttons) {
  (void)dx;
  (void)dy;
  (void)wheel;
  (void)buttons;
}
