#pragma once
#include <Arduino.h>
#include <array>

// Enumerated SX1509-sourced button inputs
enum class Button : uint8_t {
  BUTTON_LEFT = 0,
  BUTTON_RIGHT,
  BUTTON_DOWN,
  BUTTON_UP,
  BUTTON_OK,
  BUTTON_RED,
  BUTTON_YELLOW,
  BUTTON_GREEN,

  COUNT
};
static constexpr uint8_t BUTTON_COUNT = static_cast<uint8_t>(Button::COUNT);

// Enumerated SX1509-sourced LED outputs
enum class LED : uint8_t
{
  LED_RED_BUTTON = 0,
  LED_YELLOW_BUTTON,
  LED_GREEN_BUTTON,
  LED_SPARE_1,
  LED_SPARE_2,
  LED_SPARE_3,
  LED_SPARE_4,
  LED_SPARE_5,

  COUNT
};
static constexpr uint8_t LED_COUNT = static_cast<uint8_t>(LED::COUNT);


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
   * Description: Configure an LED mode by enumeration.
   * Inputs:
   * - led: LED to configure.
   * - mode: desired mode.
   * - tOnMs: on-time for blink mode.
   * - tOffMs: off-time for blink mode.
   * Outputs: Updates LED mode and timing.
   */
  void setLedMode(LED led, LedMode mode, unsigned long tOnMs = 200, unsigned long tOffMs = 800);

  /**
   * Description: Get the current mode for an LED.
   * Inputs:
   * - led: LED to query.
   * Outputs: Returns the current LED mode.
   */
  LedMode getLedMode(LED led) const;

  /**
   * Description: Set a steady LED duty cycle.
   * Inputs:
   * - led: LED to configure.
   * - duty: duty cycle (0-255).
   * Outputs: Updates LED brightness.
   */
  void setLedSteady(LED led, uint8_t duty); // 0-255 steady (inverted)

  /**
   * Description: Set an LED to blink with custom timing.
   * Inputs:
   * - led: LED to configure.
   * - tOnMs: on-time in milliseconds.
   * - tOffMs: off-time in milliseconds.
   * Outputs: Updates LED mode and timing.
   */
  void setLedBlink(LED led, unsigned long tOnMs, unsigned long tOffMs);

private:
  bool _last[BUTTON_COUNT] = {true,true,true,true,true,true,true,true}; // active-low buttons, idle high
  LedMode _ledModes[LED_COUNT] = {LedMode::Off,LedMode::Off,LedMode::Off,LedMode::Off,LedMode::Off,LedMode::Off,LedMode::Off,LedMode::Off};
  uint8_t _ledDuty[LED_COUNT] = {0,0,0,0,0,0,0,0}; // logical 0-255 (inverted when written)
  unsigned long _ledOnMs[LED_COUNT] = {0};
  unsigned long _ledOffMs[LED_COUNT] = {0};
  bool _ledPhaseOn[LED_COUNT] = {false,false,false,false,false,false,false,false};
  unsigned long _ledLastToggleMs[LED_COUNT] = {0};

  /**
   * Description: Advance LED blink state machine and write outputs.
   * Inputs: None.
   * Outputs: Updates LED driver outputs.
   */
  void updateLeds();
};
