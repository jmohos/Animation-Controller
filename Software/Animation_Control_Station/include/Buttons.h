#pragma once
#include <Arduino.h>
#include <array>

// Enumerated SX1509-sourced button inputs
enum class Button : uint8_t
{
  BUTTON_OK = 0,
  BUTTON_DOWN,
  BUTTON_UP,
  BUTTON_LEFT,
  BUTTON_RIGHT,
  BUTTON_RED,
  BUTTON_YELLOW,
  BUTTON_GREEN,

  COUNT
};
static constexpr uint8_t BUTTON_COUNT = static_cast<uint8_t>(Button::COUNT);

struct ButtonState {
  // indexed by Button enum
  std::array<bool, BUTTON_COUNT> isPressed{};       // current debounced state
  std::array<bool, BUTTON_COUNT> isJustPressed{};   // rising edge this poll
  std::array<bool, BUTTON_COUNT> isJustReleased{};  // falling edge this poll

  /**
   * Description: Check if a button is currently pressed.
   * Inputs:
   * - button: button to query.
   * Outputs: Returns true when pressed.
   */
  inline bool pressed(Button button) const { return isPressed[static_cast<uint8_t>(button)]; }

  /**
   * Description: Check if a button was pressed this poll cycle.
   * Inputs:
   * - button: button to query.
   * Outputs: Returns true on rising edge.
   */
  inline bool justPressed(Button button) const { return isJustPressed[static_cast<uint8_t>(button)]; }

  /**
   * Description: Check if a button was released this poll cycle.
   * Inputs:
   * - button: button to query.
   * Outputs: Returns true on falling edge.
   */
  inline bool justReleased(Button button) const { return isJustReleased[static_cast<uint8_t>(button)]; }
};

class Buttons {
public:
  /**
   * Description: Initialize the SX1509 buttons.
   * Inputs: None.
   * Outputs: Returns true on success, false on initialization failure.
   */
  bool begin();

  /**
   * Description: Poll buttons into a snapshot.
   * Inputs: None.
   * Outputs: Returns the current button state.
   */
  ButtonState poll();

private:
  bool _last[BUTTON_COUNT] = {true,true,true,true,true,true,true,true}; // active-low buttons, idle high
};
