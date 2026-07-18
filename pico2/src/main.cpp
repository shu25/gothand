#include <Arduino.h>

#include "Buttons.h"
#include "Encoder.h"
#include "EspProtocol.h"
#include "MouseController.h"
#include "RfManager.h"
#include "Trackball.h"
#include "config.h"

QuadratureTrackball trackball(
    TRACKBALL_XA_PIN,
    TRACKBALL_XB_PIN,
    TRACKBALL_YA_PIN,
    TRACKBALL_YB_PIN);
Encoder scrollEncoder(SCROLL_ENCODER_A_PIN, SCROLL_ENCODER_B_PIN);
Buttons buttons;
MouseController mouse;
EspProtocol espProtocol;
RfManager rfManager;

uint32_t lastMouseReport = 0;
uint32_t lastStatus = 0;

void setup() {
  Serial.begin(SERIAL_BAUD);
  Serial1.setTX(ESP_UART_TX_PIN);
  Serial1.setRX(ESP_UART_RX_PIN);
  Serial1.begin(ESP_UART_BAUD);

  trackball.begin();
  scrollEncoder.begin();
  buttons.begin();
  mouse.begin();
  espProtocol.begin(Serial1);
  rfManager.begin();

  DEBUG_LOG(F("Godhand Pico 2 mouse controller ready"));
}

void loop() {
  const uint32_t now = millis();

  trackball.update();
  scrollEncoder.update();
  espProtocol.update();
  rfManager.update();

  const ButtonEvent buttonEvent = buttons.update();
  if (buttonEvent.valid) {
    mouse.handleButton(buttonEvent);
    espProtocol.sendButton(buttonEvent.index, buttonEvent.pressed);
  }

  if (now - lastMouseReport >= MOUSE_REPORT_INTERVAL_MS) {
    lastMouseReport = now;
    const int dx = trackball.getDeltaX() * POINTER_SCALE;
    const int dy = trackball.getDeltaY() * POINTER_SCALE;
    const int wheel = scrollEncoder.readSteps() * SCROLL_SCALE;
    mouse.move(dx, dy, wheel);
    espProtocol.sendMouseDelta(dx, dy);
    espProtocol.sendScroll(wheel);
  }

  if (now - lastStatus >= STATUS_INTERVAL_MS) {
    lastStatus = now;
    espProtocol.sendStatus();
  }
}
