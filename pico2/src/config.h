#pragma once

#include <Arduino.h>

constexpr uint32_t SERIAL_BAUD = 115200;
constexpr uint32_t ESP_UART_BAUD = 115200;

constexpr int ESP_UART_TX_PIN = 0;
constexpr int ESP_UART_RX_PIN = 1;

constexpr uint8_t TRACKBALL_XA_PIN = 2;
constexpr uint8_t TRACKBALL_XB_PIN = 3;
constexpr uint8_t TRACKBALL_YA_PIN = 4;
constexpr uint8_t TRACKBALL_YB_PIN = 5;

constexpr uint8_t SCROLL_ENCODER_A_PIN = 6;
constexpr uint8_t SCROLL_ENCODER_B_PIN = 7;

constexpr uint8_t MOUSE_BUTTON_COUNT = 10;
constexpr uint8_t MOUSE_BUTTON_PINS[MOUSE_BUTTON_COUNT] = {
    8, 9, 10, 11, 12, 13, 14, 15, 16, 17};

constexpr uint32_t INPUT_DEBOUNCE_MS = 20;
constexpr uint32_t MOUSE_REPORT_INTERVAL_MS = 1;
constexpr uint32_t STATUS_INTERVAL_MS = 1000;

constexpr int POINTER_SCALE = 1;
constexpr int SCROLL_SCALE = 1;

#define ENABLE_USB_MOUSE 1
constexpr bool DEBUG_LOG_ENABLED = true;

#define DEBUG_LOG(message)       \
  do {                           \
    if (DEBUG_LOG_ENABLED) {     \
      Serial.println(message);   \
    }                            \
  } while (false)
