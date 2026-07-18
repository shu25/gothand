#pragma once

#include <Arduino.h>

#include "AppState.h"

constexpr uint32_t SERIAL_BAUD = 115200;

constexpr int SCREEN_WIDTH = 128;
constexpr int SCREEN_HEIGHT = 32;
constexpr int OLED_RESET_PIN = -1;
constexpr uint8_t OLED_I2C_ADDRESS = 0x3C;
constexpr uint8_t I2C_SDA_PIN = 6;
constexpr uint8_t I2C_SCL_PIN = 7;

constexpr uint8_t ENCODER_A_PIN = 8;
constexpr uint8_t ENCODER_B_PIN = 9;
constexpr uint8_t ENCODER_SW_PIN = 10;
constexpr int ENCODER_VOLUME_STEP = 5;

constexpr uint8_t BUTTON_COUNT = 5;
constexpr uint8_t BUTTON_PINS[BUTTON_COUNT] = {0, 1, 2, 3, 4};
constexpr ButtonAction BUTTON_ACTIONS[BUTTON_COUNT] = {
    ButtonAction::PLAY_PAUSE,
    ButtonAction::PREVIOUS_TRACK,
    ButtonAction::NEXT_TRACK,
    ButtonAction::MODE_CHANGE,
    ButtonAction::MUTE,
};

constexpr uint32_t PICO_UART_BAUD = 115200;
constexpr int PICO_UART_RX_PIN = 21;
constexpr int PICO_UART_TX_PIN = 20;

constexpr uint32_t INPUT_DEBOUNCE_MS = 25;
constexpr uint32_t DISPLAY_REFRESH_MS = 80;
constexpr uint32_t VOLUME_SCREEN_TIMEOUT_MS = 2500;
constexpr uint32_t TELEMETRY_INTERVAL_MS = 250;
constexpr uint32_t PICO_LINK_TIMEOUT_MS = 2500;
constexpr uint8_t DEFAULT_FFT_BANDS = 16;
constexpr uint8_t MAX_FFT_BANDS = 32;

constexpr const char* BLE_DEVICE_NAME = "Godhand Controller";
constexpr const char* SERVICE_UUID = "12345678-1234-1234-1234-1234567890ab";
constexpr const char* CMD_UUID = "12345678-1234-1234-1234-1234567890ac";
constexpr const char* STATE_UUID = "12345678-1234-1234-1234-1234567890ad";

constexpr bool DEBUG_LOG_ENABLED = true;

#define DEBUG_LOG(message)       \
  do {                           \
    if (DEBUG_LOG_ENABLED) {     \
      Serial.println(message);   \
    }                            \
  } while (false)
