#include "MouseController.h"

#include "config.h"

#if ENABLE_USB_MOUSE
#include <Mouse.h>
#endif

void MouseController::begin() {
#if ENABLE_USB_MOUSE
  Mouse.begin();
#endif
}

void MouseController::move(int dx, int dy, int wheel) {
  if (dx == 0 && dy == 0 && wheel == 0) {
    return;
  }

#if ENABLE_USB_MOUSE
  Mouse.move(dx, dy, wheel);
#else
  Serial.print(F("MOUSE:"));
  Serial.print(dx);
  Serial.print(F(","));
  Serial.print(dy);
  Serial.print(F(" SCROLL:"));
  Serial.println(wheel);
#endif
}

void MouseController::handleButton(const ButtonEvent& event) {
  if (!event.valid) {
    return;
  }

  const uint8_t mask = buttonMask(event.action);
  if (mask == 0) {
    Serial.print(F("BTN:"));
    Serial.print(event.index + 1);
    Serial.println(event.pressed ? F(":DOWN") : F(":UP"));
    return;
  }

#if ENABLE_USB_MOUSE
  if (event.pressed) {
    Mouse.press(mask);
  } else {
    Mouse.release(mask);
  }
#endif
}

uint8_t MouseController::buttonMask(MouseButtonAction action) const {
  switch (action) {
    case MouseButtonAction::LEFT:
      return MOUSE_LEFT;
    case MouseButtonAction::RIGHT:
      return MOUSE_RIGHT;
    case MouseButtonAction::MIDDLE:
      return MOUSE_MIDDLE;
    default:
      return 0;
  }
}
