#pragma once
#include <Arduino.h>

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

enum class LedMode : uint8_t {
  Off = 0,
  On,
  Blink
};

class Leds {
public:
  /**
   * Description: Initialize the SX1509 LED drivers.
   * Inputs: None.
   * Outputs: Returns true on success, false on initialization failure.
   */
  bool begin();

  /**
   * Description: Advance LED blink state machine and write outputs.
   * Inputs: None.
   * Outputs: Updates LED driver outputs.
   */
  void update();

  /**
   * Description: Configure an LED mode.
   * Inputs:
   * - led: LED to configure.
   * - mode: desired mode.
   * - tOnMs: on-time for blink mode.
   * - tOffMs: off-time for blink mode.
   * Outputs: Updates LED mode and timing.
   */
  void setMode(LED led, LedMode mode, unsigned long tOnMs = 200, unsigned long tOffMs = 800);

  /**
   * Description: Get the current mode for an LED.
   * Inputs:
   * - led: LED to query.
   * Outputs: Returns the current LED mode.
   */
  LedMode getMode(LED led) const;

  /**
   * Description: Set a steady LED duty cycle.
   * Inputs:
   * - led: LED to configure.
   * - duty: duty cycle (0-255).
   * Outputs: Updates LED brightness.
   */
  void setSteady(LED led, uint8_t duty); // 0-255 steady (inverted)

  /**
   * Description: Set an LED to blink with custom timing.
   * Inputs:
   * - led: LED to configure.
   * - tOnMs: on-time in milliseconds.
   * - tOffMs: off-time in milliseconds.
   * Outputs: Updates LED mode and timing.
   */
  void setBlink(LED led, unsigned long tOnMs, unsigned long tOffMs);

private:
  LedMode _ledModes[LED_COUNT] = {LedMode::Off,LedMode::Off,LedMode::Off,LedMode::Off,LedMode::Off,LedMode::Off,LedMode::Off,LedMode::Off};
  uint8_t _ledDuty[LED_COUNT] = {0,0,0,0,0,0,0,0}; // logical 0-255 (inverted when written)
  unsigned long _ledOnMs[LED_COUNT] = {0};
  unsigned long _ledOffMs[LED_COUNT] = {0};
  bool _ledPhaseOn[LED_COUNT] = {false,false,false,false,false,false,false,false};
  unsigned long _ledLastToggleMs[LED_COUNT] = {0};
};
