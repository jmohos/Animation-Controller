#pragma once
#include <Arduino.h>
#include <array>
#include "BoardPins.h"

// Enumerated SX1509-sourced button inputs
enum class Button : uint8_t {
  Mode = 0,
  Up,
  Down,
  Left,
  Right,
  OK,
  Play,
  Spare,
  Count
};
static constexpr uint8_t BUTTON_COUNT = static_cast<uint8_t>(Button::Count);


struct InputState {
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

  // analog
  float potSpeedNorm = 0.0f; // 0..1
  float potAccelNorm = 0.0f; // 0..1

  // jog encoder
  int32_t encoderDelta = 0;
};

enum class LedMode : uint8_t {
  Off = 0,
  On,
  Blink
};

class Input {
public:
  /**
   * Description: Initialize I2C expander, buttons, LEDs, and ADC settings.
   * Inputs: None.
   * Outputs: Returns true on success, false on initialization failure.
   */
  bool begin();

  /**
   * Description: Poll buttons, LEDs, and potentiometers into a snapshot.
   * Inputs: None.
   * Outputs: Returns the current input state.
   */
  InputState poll();

  /**
   * Description: Set the play LED to on or off.
   * Inputs:
   * - on: true to illuminate, false to turn off.
   * Outputs: Updates LED state.
   */
  void setPlayLed(bool on);

  /**
   * Description: Configure an LED mode by index.
   * Inputs:
   * - idx: LED index [0..7].
   * - mode: desired mode.
   * - tOnMs: on-time for blink mode.
   * - tOffMs: off-time for blink mode.
   * Outputs: Updates LED mode and timing.
   */
  void setLedMode(uint8_t idx, LedMode mode, unsigned long tOnMs = 200, unsigned long tOffMs = 800);

  /**
   * Description: Configure an LED mode using the SXPin enum.
   * Inputs:
   * - pin: SXPin LED entry.
   * - mode: desired mode.
   * - tOnMs: on-time for blink mode.
   * - tOffMs: off-time for blink mode.
   * Outputs: Updates LED mode and timing.
   */
  void setLedMode(SXPin pin, LedMode mode, unsigned long tOnMs = 200, unsigned long tOffMs = 800) { setLedMode((uint8_t)pin - 8, mode, tOnMs, tOffMs); }

  /**
   * Description: Get the current mode for an LED by index.
   * Inputs:
   * - idx: LED index [0..7].
   * Outputs: Returns the current LED mode.
   */
  LedMode getLedMode(uint8_t idx) const;

  /**
   * Description: Get the current mode for an LED using the SXPin enum.
   * Inputs:
   * - pin: SXPin LED entry.
   * Outputs: Returns the current LED mode.
   */
  LedMode getLedMode(SXPin pin) const { return getLedMode((uint8_t)pin - 8); }

  /**
   * Description: Set a steady LED duty cycle.
   * Inputs:
   * - idx: LED index [0..7].
   * - duty: duty cycle (0-255).
   * Outputs: Updates LED brightness.
   */
  void setLedSteady(uint8_t idx, uint8_t duty); // 0-255 steady (inverted)

  /**
   * Description: Set an LED to blink with custom timing.
   * Inputs:
   * - idx: LED index [0..7].
   * - tOnMs: on-time in milliseconds.
   * - tOffMs: off-time in milliseconds.
   * Outputs: Updates LED mode and timing.
   */
  void setLedBlink(uint8_t idx, unsigned long tOnMs, unsigned long tOffMs);

private:
  bool _last[8] = {true,true,true,true,true,true,true,true}; // active-low buttons, idle high
  LedMode _ledModes[8] = {LedMode::Off,LedMode::Off,LedMode::Off,LedMode::Off,LedMode::Off,LedMode::Off,LedMode::Off,LedMode::Off};
  uint8_t _ledDuty[8] = {0,0,0,0,0,0,0,0}; // logical 0-255 (inverted when written)
  unsigned long _ledOnMs[8] = {0};
  unsigned long _ledOffMs[8] = {0};
  bool _ledPhaseOn[8] = {false,false,false,false,false,false,false,false};
  unsigned long _ledLastToggleMs[8] = {0};

  /**
   * Description: Advance LED blink state machine and write outputs.
   * Inputs: None.
   * Outputs: Updates LED driver outputs.
   */
  void updateLeds();
};
