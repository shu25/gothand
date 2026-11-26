#include "DisplayUi.h"

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

#include "config.h"

namespace {
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET_PIN);

constexpr int TEXT_SIZE_1_WIDTH = 6;
constexpr int HEADER_HEIGHT = 10;
constexpr int PORTRAIT_HEADER_HEIGHT = 20;

int textWidth(const String& text, uint8_t size = 1) {
  return text.length() * TEXT_SIZE_1_WIDTH * size;
}

bool portrait() {
  return display.width() < display.height();
}

int headerHeight() {
  return portrait() ? PORTRAIT_HEADER_HEIGHT : HEADER_HEIGHT;
}

void drawTextRight(const String& text, int right, int y, uint8_t size = 1) {
  display.setTextSize(size);
  display.setCursor(max(0, right - textWidth(text, size)), y);
  display.print(text);
}

void drawTextCentered(const String& text, int y, uint8_t size = 1) {
  display.setTextSize(size);
  display.setCursor(max(0, (display.width() - textWidth(text, size)) / 2), y);
  display.print(text);
}

void drawBluetoothIcon(int x, int y, bool connected) {
  display.drawFastVLine(x + 3, y, 9, SSD1306_WHITE);
  display.drawLine(x + 3, y, x + 6, y + 3, SSD1306_WHITE);
  display.drawLine(x + 6, y + 3, x + 3, y + 5, SSD1306_WHITE);
  display.drawLine(x + 3, y + 5, x + 6, y + 8, SSD1306_WHITE);
  display.drawLine(x + 6, y + 8, x + 3, y + 5, SSD1306_WHITE);
  display.drawLine(x + 3, y + 5, x, y + 8, SSD1306_WHITE);
  display.drawLine(x + 3, y + 5, x, y + 1, SSD1306_WHITE);
  if (!connected) {
    display.drawLine(x, y + 9, x + 7, y, SSD1306_WHITE);
  }
}

void drawSpeakerIcon(int x, int y, bool muted) {
  display.fillRect(x, y + 4, 2, 4, SSD1306_WHITE);
  display.drawLine(x + 2, y + 4, x + 5, y + 1, SSD1306_WHITE);
  display.drawLine(x + 2, y + 7, x + 5, y + 10, SSD1306_WHITE);
  display.drawFastVLine(x + 5, y + 1, 10, SSD1306_WHITE);
  if (muted) {
    display.drawLine(x + 8, y + 2, x + 12, y + 9, SSD1306_WHITE);
  } else {
    display.drawPixel(x + 8, y + 4, SSD1306_WHITE);
    display.drawPixel(x + 9, y + 5, SSD1306_WHITE);
    display.drawPixel(x + 9, y + 6, SSD1306_WHITE);
    display.drawPixel(x + 8, y + 7, SSD1306_WHITE);
  }
}

void drawSmallSpeakerIcon(int x, int y, bool muted) {
  display.fillRect(x, y + 3, 2, 3, SSD1306_WHITE);
  display.drawLine(x + 2, y + 3, x + 5, y + 1, SSD1306_WHITE);
  display.drawLine(x + 2, y + 5, x + 5, y + 7, SSD1306_WHITE);
  display.drawFastVLine(x + 5, y + 1, 7, SSD1306_WHITE);
  if (muted) {
    display.drawLine(x + 7, y + 1, x + 11, y + 7, SSD1306_WHITE);
  } else {
    display.drawPixel(x + 8, y + 3, SSD1306_WHITE);
    display.drawPixel(x + 9, y + 4, SSD1306_WHITE);
    display.drawPixel(x + 8, y + 5, SSD1306_WHITE);
  }
}

void drawPlaybackStateIcon(int x, int y, bool playing) {
  if (playing) {
    display.fillTriangle(x, y, x, y + 8, x + 7, y + 4, SSD1306_WHITE);
  } else {
    display.fillRect(x + 1, y + 1, 7, 7, SSD1306_WHITE);
  }
}

void drawArrowIcon(int x, int y, bool next) {
  if (next) {
    display.fillTriangle(x + 2, y, x + 2, y + 8, x + 7, y + 4, SSD1306_WHITE);
    display.drawFastVLine(x + 9, y, 9, SSD1306_WHITE);
  } else {
    display.fillTriangle(x + 8, y, x + 8, y + 8, x + 3, y + 4, SSD1306_WHITE);
    display.drawFastVLine(x + 1, y, 9, SSD1306_WHITE);
  }
}

void drawModeIcon(int x, int y) {
  display.drawCircle(x + 4, y + 4, 4, SSD1306_WHITE);
  display.drawPixel(x + 8, y + 2, SSD1306_WHITE);
  display.drawPixel(x + 7, y + 1, SSD1306_WHITE);
  display.drawPixel(x, y + 6, SSD1306_WHITE);
  display.drawPixel(x + 1, y + 7, SSD1306_WHITE);
}

void drawFeedbackIcon(const AppState& state, uint32_t now) {
  if (state.feedbackIcon == FeedbackIcon::NONE || static_cast<int32_t>(now - state.feedbackUntil) >= 0) {
    return;
  }

  const int x = portrait() ? display.width() - 14 : display.width() - 14;
  const int y = portrait() ? headerHeight() + 3 : headerHeight() + 2;

  switch (state.feedbackIcon) {
    case FeedbackIcon::PLAYBACK:
      drawPlaybackStateIcon(x + 1, y, state.audioPlaying);
      break;
    case FeedbackIcon::PREVIOUS:
      drawArrowIcon(x, y, false);
      break;
    case FeedbackIcon::NEXT:
      drawArrowIcon(x, y, true);
      break;
    case FeedbackIcon::MODE:
      drawModeIcon(x + 1, y);
      break;
    case FeedbackIcon::MUTE:
      drawSmallSpeakerIcon(x - 1, y, true);
      break;
    case FeedbackIcon::VOLUME_UP:
      drawSmallSpeakerIcon(x - 2, y, false);
      display.drawFastHLine(x + 8, y + 2, 5, SSD1306_WHITE);
      display.drawFastVLine(x + 10, y, 5, SSD1306_WHITE);
      break;
    case FeedbackIcon::VOLUME_DOWN:
      drawSmallSpeakerIcon(x - 2, y, false);
      display.drawFastHLine(x + 8, y + 5, 5, SSD1306_WHITE);
      break;
    case FeedbackIcon::NONE:
      break;
  }
}

String volumeLabel(const AppState& state) {
  if (state.muted) {
    return F("M");
  }
  String text = String(state.volume);
  return text;
}

void drawMiniAnalogClock(uint8_t hours, uint8_t minutes, uint8_t seconds) {
  const int cx = display.width() / 2;
  const int cy = 106;
  const int radius = 13;

  display.drawCircle(cx, cy, radius, SSD1306_WHITE);
  display.drawPixel(cx, cy - radius + 2, SSD1306_WHITE);
  display.drawPixel(cx + radius - 2, cy, SSD1306_WHITE);
  display.drawPixel(cx, cy + radius - 2, SSD1306_WHITE);
  display.drawPixel(cx - radius + 2, cy, SSD1306_WHITE);

  const uint16_t minuteAngle = minutes * 6;
  const uint16_t hourAngle = ((hours % 12) * 30) + (minutes / 2);
  const uint16_t secondAngle = seconds * 6;

  const float minuteRad = (minuteAngle - 90) * DEG_TO_RAD;
  const float hourRad = (hourAngle - 90) * DEG_TO_RAD;
  const float secondRad = (secondAngle - 90) * DEG_TO_RAD;

  display.drawLine(cx, cy, cx + cos(hourRad) * 6, cy + sin(hourRad) * 6, SSD1306_WHITE);
  display.drawLine(cx, cy, cx + cos(minuteRad) * 10, cy + sin(minuteRad) * 10, SSD1306_WHITE);
  display.drawPixel(cx + cos(secondRad) * 11, cy + sin(secondRad) * 11, SSD1306_WHITE);
  display.fillCircle(cx, cy, 1, SSD1306_WHITE);
}

void drawHeader(const AppState& state) {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  if (portrait()) {
    drawBluetoothIcon(0, 0, state.bluetoothConnected);
    display.setCursor(12, 0);
    display.print(state.bluetoothConnected ? F("ON") : F("OFF"));
    drawSpeakerIcon(0, 8, state.muted);
    drawTextRight(volumeLabel(state), display.width(), 10);
    display.drawFastHLine(0, headerHeight() - 1, display.width(), SSD1306_WHITE);
    return;
  }

  drawBluetoothIcon(0, 0, state.bluetoothConnected);

  display.setCursor(34, 0);
  display.print(state.audioPlaying ? F("STOP") : F("PLAY"));

  drawSpeakerIcon(display.width() - 48, -1, state.muted);
  drawTextRight(volumeLabel(state), display.width(), 0);
  display.drawFastHLine(0, headerHeight() - 1, display.width(), SSD1306_WHITE);
}

void drawFftHeader(const AppState& state, uint32_t now) {
  const uint32_t seconds = state.currentClockSeconds(now);
  const uint8_t hours = (seconds / 3600UL) % 24;
  const uint8_t minutes = (seconds / 60UL) % 60;

  char clockBuffer[6];
  snprintf(clockBuffer, sizeof(clockBuffer), "%02u:%02u", hours, minutes);

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  if (portrait()) {
    drawTextCentered(String(clockBuffer), 0);
    drawSpeakerIcon(0, 8, state.muted);
    drawTextRight(volumeLabel(state), display.width(), 10);
    display.drawFastHLine(0, headerHeight() - 1, display.width(), SSD1306_WHITE);
    return;
  }

  drawBluetoothIcon(0, 0, state.bluetoothConnected);
  drawTextCentered(String(clockBuffer), 0);

  drawSpeakerIcon(display.width() - 48, -1, state.muted);
  drawTextRight(volumeLabel(state), display.width(), 0);
  display.drawFastHLine(0, headerHeight() - 1, display.width(), SSD1306_WHITE);
}

void drawClock(const AppState& state, uint32_t now) {
  const uint32_t seconds = state.currentClockSeconds(now);
  const uint8_t hours = (seconds / 3600UL) % 24;
  const uint8_t minutes = (seconds / 60UL) % 60;
  const uint8_t clockSeconds = seconds % 60;

  char buffer[9];
  snprintf(buffer, sizeof(buffer), "%02u:%02u:%02u", hours, minutes, clockSeconds);

  display.setTextColor(SSD1306_WHITE);
  if (portrait()) {
    char hourBuffer[3];
    char minuteBuffer[3];
    char secondBuffer[3];
    snprintf(hourBuffer, sizeof(hourBuffer), "%02u", hours);
    snprintf(minuteBuffer, sizeof(minuteBuffer), "%02u", minutes);
    snprintf(secondBuffer, sizeof(secondBuffer), "%02u", clockSeconds);
    drawHeader(state);
    drawTextCentered(String(hourBuffer), 34, 2);
    drawTextCentered(String(minuteBuffer), 65, 2);
    drawTextRight(String(secondBuffer), display.width(), 80);
    drawMiniAnalogClock(hours, minutes, clockSeconds);
    return;
  }

  drawTextCentered(buffer, display.height() >= 64 ? 24 : 14, display.height() >= 64 ? 3 : 2);

  drawHeader(state);
}

void drawVolume(const AppState& state) {
  drawHeader(state);

  display.setTextSize(1);
  if (portrait()) {
    const int barX = 8;
    const int barY = 37;
    const int barW = 16;
    const int barH = 68;
    const int fillH = map(state.muted ? 0 : state.volume, 0, 100, 0, barH - 2);

    display.setCursor(7, 23);
    display.print(F("VOL"));
    display.drawRect(barX, barY, barW, barH, SSD1306_WHITE);
    display.fillRect(barX + 1, barY + barH - 1 - fillH, barW - 2, fillH, SSD1306_WHITE);

    if (state.muted) {
      drawTextCentered(F("MUTE"), 112);
    } else {
      drawTextCentered(String(state.volume) + F("%"), 112);
    }
    return;
  }

  display.setCursor(0, 13);
  display.print(F("VOL"));

  const int barX = 26;
  const int barY = 14;
  const int barW = display.width() - barX - 2;
  const int barH = 9;
  const int fillW = map(state.muted ? 0 : state.volume, 0, 100, 0, barW - 2);

  display.drawRect(barX, barY, barW, barH, SSD1306_WHITE);
  display.fillRect(barX + 1, barY + 1, fillW, barH - 2, SSD1306_WHITE);

  if (state.muted) {
    drawTextCentered(F("MUTE"), 24);
  } else {
    drawTextCentered(String(state.volume) + F("%"), 24);
  }
}

uint8_t fakeFftValue(uint8_t index, uint32_t now) {
  const uint8_t phase = (now / 90 + index * 17) % 64;
  const uint8_t triangle = phase < 32 ? phase : 63 - phase;
  const uint8_t ripple = (index * 11 + now / 230) % 18;
  return constrain((triangle * 3) + ripple, 6, 100);
}

void drawFft(AppState& state, uint32_t now) {
  drawFftHeader(state, now);

  const int top = headerHeight() + 4;
  const int bottom = display.height() - 4;
  const int usableHeight = bottom - top;
  const int barWidth = portrait() ? 6 : 18;
  const int gap = portrait() ? 4 : 8;
  const int totalWidth = barWidth * 3 + gap * 2;
  const int startX = max(0, (display.width() - totalWidth) / 2);
  const uint8_t sourceBands = constrain(state.fftBandCount, 1, MAX_FFT_BANDS);

  for (uint8_t i = 0; i < 3; ++i) {
    uint16_t sum = 0;
    uint8_t count = 0;
    const uint8_t start = (sourceBands * i) / 3;
    const uint8_t end = max<uint8_t>(start + 1, (sourceBands * (i + 1)) / 3);
    for (uint8_t band = start; band < end && band < sourceBands; ++band) {
      sum += state.fftDataValid ? state.fftBars[band] : fakeFftValue(band, now);
      ++count;
    }
    const uint8_t value = count == 0 ? 0 : sum / count;
    const int barHeight = map(constrain(value, 0, 100), 0, 100, 1, usableHeight);
    const int x = startX + i * (barWidth + gap);
    const int y = bottom - barHeight;
    display.fillRect(x, y, barWidth, barHeight, SSD1306_WHITE);
  }
}

void drawWave(AppState& state, uint32_t now) {
  drawFftHeader(state, now);

  const uint8_t points = constrain(state.fftBandCount, 2, MAX_FFT_BANDS);
  const int top = HEADER_HEIGHT + 2;
  const int bottom = SCREEN_HEIGHT - 1;
  const int mid = (top + bottom) / 2;
  const int amplitude = max(1, (bottom - top) / 2);
  const int step = max(1, (SCREEN_WIDTH - 1) / (points - 1));

  int previousX = 0;
  int previousY = mid;

  for (uint8_t i = 0; i < points; ++i) {
    const uint8_t value = state.fftDataValid ? state.fftBars[i] : fakeFftValue(i, now);
    const int offset = map(constrain(value, 0, 100), 0, 100, -amplitude, amplitude);
    const int x = i == points - 1 ? SCREEN_WIDTH - 1 : i * step;
    const int y = constrain(mid + offset, top, bottom);

    if (i > 0) {
      display.drawLine(previousX, previousY, x, y, SSD1306_WHITE);
      const int glowY = constrain((previousY + y) / 2 + (i % 2 == 0 ? 1 : -1), top, bottom);
      display.drawPixel((previousX + x) / 2, glowY, SSD1306_WHITE);
    }

    previousX = x;
    previousY = y;
  }
}

void drawDisc(AppState& state, uint32_t now) {
  drawFftHeader(state, now);

  uint16_t levelSum = 0;
  const uint8_t points = constrain(state.fftBandCount, 1, MAX_FFT_BANDS);
  for (uint8_t i = 0; i < points; ++i) {
    levelSum += state.fftDataValid ? state.fftBars[i] : fakeFftValue(i, now);
  }
  const uint8_t level = levelSum / points;

  if (portrait()) {
    const int cx = display.width() / 2;
    const int cy = 48;
    const int outer = 12 + (state.audioPlaying ? map(level, 0, 100, 0, 1) : 0);

    display.drawCircle(cx, cy, outer, SSD1306_WHITE);
    display.drawCircle(cx, cy, outer - 4, SSD1306_WHITE);
    display.drawCircle(cx, cy, outer - 7, SSD1306_WHITE);
    display.fillCircle(cx, cy, 2, SSD1306_WHITE);
    display.drawPixel(cx, cy, SSD1306_BLACK);

    const uint8_t phase = state.audioPlaying ? (now / 110) % 8 : 0;
    const int markerRadius = outer - 1;
    int markerX = cx;
    int markerY = cy - markerRadius;
    if (phase == 1) {
      markerX = cx + 6;
      markerY = cy - 8;
    } else if (phase == 2) {
      markerX = cx + markerRadius;
      markerY = cy;
    } else if (phase == 3) {
      markerX = cx + 6;
      markerY = cy + 8;
    } else if (phase == 4) {
      markerX = cx;
      markerY = cy + markerRadius;
    } else if (phase == 5) {
      markerX = cx - 6;
      markerY = cy + 8;
    } else if (phase == 6) {
      markerX = cx - markerRadius;
      markerY = cy;
    } else if (phase == 7) {
      markerX = cx - 6;
      markerY = cy - 8;
    }
    display.fillCircle(markerX, markerY, 1, SSD1306_WHITE);

    const int armBaseX = 27;
    const int armBaseY = 33;
    const int needleX = state.audioPlaying ? cx + 8 : cx + 12;
    const int needleY = state.audioPlaying ? cy - 5 + map(level, 0, 100, 0, 2) : cy - 11;
    display.fillCircle(armBaseX, armBaseY, 2, SSD1306_WHITE);
    display.drawLine(armBaseX - 1, armBaseY + 1, needleX, needleY, SSD1306_WHITE);
    display.fillCircle(needleX, needleY, 1, SSD1306_WHITE);

    drawPlaybackStateIcon((display.width() - 8) / 2, 69, state.audioPlaying);

    const int barTop = 85;
    const int barBottom = display.height() - 5;
    const int barHeightMax = barBottom - barTop;
    const int barWidth = 6;
    const int gap = 4;
    const int startX = 3;
    const uint8_t sourceBands = constrain(state.fftBandCount, 1, MAX_FFT_BANDS);
    for (uint8_t i = 0; i < 3; ++i) {
      uint16_t sum = 0;
      uint8_t count = 0;
      const uint8_t start = (sourceBands * i) / 3;
      const uint8_t end = max<uint8_t>(start + 1, (sourceBands * (i + 1)) / 3);
      for (uint8_t band = start; band < end && band < sourceBands; ++band) {
        sum += state.fftDataValid ? state.fftBars[band] : fakeFftValue(band, now);
        ++count;
      }
      const uint8_t value = state.audioPlaying && count > 0 ? sum / count : 0;
      const int barHeight = map(constrain(value, 0, 100), 0, 100, 1, barHeightMax);
      const int x = startX + i * (barWidth + gap);
      display.fillRect(x, barBottom - barHeight, barWidth, barHeight, SSD1306_WHITE);
    }
    return;
  }

  const int cx = 28;
  const int cy = 22;
  const int pulse = state.audioPlaying ? map(level, 0, 100, 0, 1) : 0;
  const int outer = 10 + pulse;

  display.drawCircle(cx, cy, outer, SSD1306_WHITE);
  display.drawCircle(cx, cy, outer - 3, SSD1306_WHITE);
  display.drawCircle(cx, cy, outer - 6, SSD1306_WHITE);
  display.fillCircle(cx, cy, 2, SSD1306_WHITE);
  display.drawPixel(cx, cy, SSD1306_BLACK);

  const uint8_t phase = state.audioPlaying ? (now / 110) % 8 : 0;
  const int markerRadius = outer - 1;
  int markerX = cx;
  int markerY = cy - markerRadius;
  if (phase == 1) {
    markerX = cx + 5;
    markerY = cy - 7;
  } else if (phase == 2) {
    markerX = cx + markerRadius;
    markerY = cy;
  } else if (phase == 3) {
    markerX = cx + 5;
    markerY = cy + 7;
  } else if (phase == 4) {
    markerX = cx;
    markerY = cy + markerRadius;
  } else if (phase == 5) {
    markerX = cx - 5;
    markerY = cy + 7;
  } else if (phase == 6) {
    markerX = cx - markerRadius;
    markerY = cy;
  } else if (phase == 7) {
    markerX = cx - 5;
    markerY = cy - 7;
  }
  display.fillCircle(markerX, markerY, 1, SSD1306_WHITE);

  const int armBaseX = 54;
  const int armBaseY = 15;
  const int needleX = state.audioPlaying ? cx + 8 : cx + 13;
  const int needleY = state.audioPlaying ? cy - 5 + map(level, 0, 100, 0, 2) : cy - 9;
  display.fillCircle(armBaseX, armBaseY, 2, SSD1306_WHITE);
  display.drawLine(armBaseX - 1, armBaseY + 1, needleX, needleY, SSD1306_WHITE);
  display.fillCircle(needleX, needleY, 1, SSD1306_WHITE);

  display.setTextSize(1);
  drawPlaybackStateIcon(74, 13, state.audioPlaying);

  const int meterX = 112;
  const int meterBottom = 30;
  for (uint8_t i = 0; i < 3; ++i) {
    const int barHeight = state.audioPlaying ? map(constrain(level + i * 12, 0, 100), 0, 100, 1, 13) : 1;
    display.drawFastVLine(meterX + i * 4, meterBottom - barHeight, barHeight, SSD1306_WHITE);
  }
}

void drawStatus(const AppState& state) {
  drawHeader(state);

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 12);
  display.print(F("PC:"));
  display.print(state.pcConnected ? F("OK") : F("--"));
  display.setCursor(54, 12);
  display.print(F("PICO:"));
  display.print(state.picoConnected ? F("OK") : F("--"));
  display.setCursor(0, 23);
  display.print(F("FFT:"));
  display.print(state.fftBandCount);
  display.setCursor(54, 23);
  display.print(F("MODE:"));
  display.print(state.controlMode);
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

  display.setRotation(1);
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
    case DisplayMode::WAVE:
    case DisplayMode::DISC:
    case DisplayMode::STATUS:
      drawDisc(state, now);
      break;
  }

  drawFeedbackIcon(state, now);
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
