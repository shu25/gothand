#include <Arduino.h>

#include "AppState.h"
#include "BleManager.h"
#include "DisplayUi.h"
#include "Inputs.h"
#include "PcProtocol.h"
#include "PicoProtocol.h"
#include "config.h"

AppState appState;
BleManager bleManager;
PcProtocol pcProtocol;
PicoProtocol picoProtocol;

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(50);

  appState.fftBandCount = DEFAULT_FFT_BANDS;

  beginInputs();
  beginDisplay();
  bleManager.begin(appState);
  Serial1.begin(PICO_UART_BAUD, SERIAL_8N1, PICO_UART_RX_PIN, PICO_UART_TX_PIN);
  picoProtocol.begin(Serial1);

  DEBUG_LOG(F("Godhand ESP32-C3 controller ready"));
}

void loop() {
  const uint32_t now = millis();

  updateInputs(appState);
  bleManager.update(appState);
  pcProtocol.update(appState);
  picoProtocol.update(appState);
  appState.update(now);
  updateDisplay(appState);
}
