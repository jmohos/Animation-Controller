#include "Buttons.h"
#include "BoardPins.h"
#include "Sx1509Bus.h"

/**
 * Description: Read a debounced button state from the SX1509 expander.
 * Inputs:
 * - sxPin: SX1509 pin index.
 * Outputs: Returns true when the button is pressed.
 */
static bool readBtn(uint8_t sxPin) {
  // buttons short to ground -> pressed = LOW
  if (!sx1509Ready()) {
    return false;
  }
  return (sx1509DigitalRead(sxPin) == LOW);
}

/**
 * Description: Configure SX1509 pins for button input with debounce.
 * Inputs: None.
 * Outputs: Sets pin modes and debounce parameters.
 */
static void initButtons() {
  sx1509DebounceTime(32); // ~32ms debounce
  for (uint8_t i = 0; i < BUTTON_COUNT; i++) {
    sx1509PinMode(i, INPUT_PULLUP);
    sx1509DebouncePin(i);
  }
}

/**
 * Description: Initialize button inputs.
 * Inputs: None.
 * Outputs: Returns true when initialization succeeds.
 */
bool Buttons::begin() {
  if (!sx1509EnsureReady()) {
    return false;
  }
  initButtons();
  return true;
}

/**
 * Description: Poll buttons and produce a snapshot of current state.
 * Inputs: None.
 * Outputs: Returns the current ButtonState snapshot.
 */
ButtonState Buttons::poll() {
  ButtonState state;
  if (!sx1509Ready()) {
    return state;
  }

  // Button order must match _last[] order (8 buttons)
  std::array<bool, BUTTON_COUNT> buttonStates = {
    readBtn(static_cast<uint8_t>(SXPin::SX_BUTTON_5)),
    readBtn(static_cast<uint8_t>(SXPin::SX_BUTTON_3)),
    readBtn(static_cast<uint8_t>(SXPin::SX_BUTTON_4)),
    readBtn(static_cast<uint8_t>(SXPin::SX_BUTTON_1)),
    readBtn(static_cast<uint8_t>(SXPin::SX_BUTTON_2)),
    readBtn(static_cast<uint8_t>(SXPin::SX_BUTTON_6)),
    readBtn(static_cast<uint8_t>(SXPin::SX_BUTTON_7)),
    readBtn(static_cast<uint8_t>(SXPin::SX_BUTTON_8)),
  };

  for (uint8_t i = 0; i < BUTTON_COUNT; i++) {
    state.isPressed[i]      = buttonStates[i];
    state.isJustPressed[i]  = (buttonStates[i] && !_last[i]);
    state.isJustReleased[i] = (_last[i] && !buttonStates[i]);
    _last[i] = buttonStates[i];
  }

  return state;
}
