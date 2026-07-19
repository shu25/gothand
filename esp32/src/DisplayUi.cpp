#include "DisplayUi.h"

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

#include "config.h"

namespace {
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET_PIN);

void drawHeader(const AppState& state) {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print(state.bluetoothConnected ? F("BT") : F("--"));
  display.setCursor(28, 0);
  display.print(state.audioPlaying ? F("PLAY") : F("PAUSE"));
  display.setCursor(88, 0);
  display.print(F("VOL"));
  if (state.muted) {
    display.print(F("M"));
  } else {
    display.print(state.volume);
  }
}

void drawClock(const AppState& state, uint32_t now) {
  const uint32_t seconds = state.currentClockSeconds(now);
  const uint8_t hours = (seconds / 3600UL) % 24;
  const uint8_t minutes = (seconds / 60UL) % 60;

  char buffer[6];
  snprintf(buffer, sizeof(buffer), "%02u:%02u", hours, minutes);

  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(SCREEN_HEIGHT >= 64 ? 3 : 2);
  display.setCursor(SCREEN_HEIGHT >= 64 ? 18 : 34, SCREEN_HEIGHT >= 64 ? 22 : 12);
  display.print(buffer);

  drawHeader(state);
}

void drawVolume(const AppState& state) {
  drawHeader(state);

  display.setTextSize(1);
  display.setCursor(0, 11);
  display.print(F("VOL"));

  const int barX = 26;
  const int barY = 13;
  const int barW = SCREEN_WIDTH - barX - 2;
  const int barH = 9;
  const int fillW = map(state.muted ? 0 : state.volume, 0, 100, 0, barW - 2);

  display.drawRect(barX, barY, barW, barH, SSD1306_WHITE);
  display.fillRect(barX + 1, barY + 1, fillW, barH - 2, SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(52, 24);
  if (state.muted) {
    display.print(F("MUTE"));
  } else {
    display.print(state.volume);
    display.print(F("%"));
  }
}

uint8_t fakeFftValue(uint8_t index, uint32_t now) {
  const uint8_t phase = (now / 90 + index * 17) % 64;
  const uint8_t triangle = phase < 32 ? phase : 63 - phase;
  const uint8_t ripple = (index * 11 + now / 230) % 18;
  return constrain((triangle * 3) + ripple, 6, 100);
}

void drawFft(AppState& state, uint32_t now) {
  drawHeader(state);

  const uint8_t bands = constrain(state.fftBandCount, 1, MAX_FFT_BANDS);
  const int top = 12;
  const int usableHeight = SCREEN_HEIGHT - top;
  const int slotWidth = max(1, SCREEN_WIDTH / bands);
  const int barWidth = max(1, slotWidth - 1);

  for (uint8_t i = 0; i < bands; ++i) {
    const uint8_t value = state.fftDataValid ? state.fftBars[i] : fakeFftValue(i, now);
    const int barHeight = map(constrain(value, 0, 100), 0, 100, 1, usableHeight);
    const int x = i * slotWidth;
    const int y = SCREEN_HEIGHT - barHeight;
    display.fillRect(x, y, barWidth, barHeight, SSD1306_WHITE);
  }
}

void drawStatus(const AppState& state) {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print(F("STATUS"));
  display.setCursor(0, 11);
  display.print(F("PC:"));
  display.print(state.pcConnected ? F("OK") : F("--"));
  display.setCursor(54, 11);
  display.print(F("PICO:"));
  display.print(state.picoConnected ? F("OK") : F("--"));
  display.setCursor(0, 23);
  display.print(F("FFT:"));
  display.print(state.fftBandCount);
  display.print(F(" VOL:"));
  display.print(state.volume);
}
}

void beginDisplay() {
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (true) {
      delay(100);
    }
  }

  display.clearDisplay();
  display.setTextWrap(false);
  display.display();
}

void updateDisplay(AppState& state) {
  const uint32_t now = millis();
  if (now - state.lastDisplayUpdate < DISPLAY_REFRESH_MS) {
    return;
  }
  state.lastDisplayUpdate = now;

  display.clearDisplay();

  switch (state.displayMode) {
    case DisplayMode::CLOCK:
      drawClock(state, now);
      break;
    case DisplayMode::VOLUME:
      drawVolume(state);
      break;
    case DisplayMode::FFT:
      drawFft(state, now);
      break;
    case DisplayMode::STATUS:
      drawStatus(state);
      break;
  }

  display.display();
}

void setClockFromHms(AppState& state, const String& hms) {
  const int first = hms.indexOf(':');
  const int second = hms.indexOf(':', first + 1);
  if (first <= 0 || second <= first) {
    return;
  }

  const int hours = hms.substring(0, first).toInt();
  const int minutes = hms.substring(first + 1, second).toInt();
  const int seconds = hms.substring(second + 1).toInt();

  if (hours < 0 || hours > 23 || minutes < 0 || minutes > 59 || seconds < 0 || seconds > 59) {
    return;
  }

  state.setClock(static_cast<uint8_t>(hours), static_cast<uint8_t>(minutes), static_cast<uint8_t>(seconds));
}
