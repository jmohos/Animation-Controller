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

  inline bool pressed(Button b) const { return isPressed[static_cast<uint8_t>(b)]; }
  inline bool justPressed(Button b) const { return isJustPressed[static_cast<uint8_t>(b)]; }
  inline bool justReleased(Button b) const { return isJustReleased[static_cast<uint8_t>(b)]; }

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
  bool begin();
  InputState poll();

  void setPlayLed(bool on);
  void setLedMode(uint8_t idx, LedMode mode, unsigned long tOnMs = 200, unsigned long tOffMs = 800);
  void setLedMode(SXPin pin, LedMode mode, unsigned long tOnMs = 200, unsigned long tOffMs = 800) { setLedMode((uint8_t)pin - 8, mode, tOnMs, tOffMs); }
  LedMode getLedMode(uint8_t idx) const;
  LedMode getLedMode(SXPin pin) const { return getLedMode((uint8_t)pin - 8); }
  void setLedSteady(uint8_t idx, uint8_t duty); // 0-255 steady (inverted)
  void setLedBlink(uint8_t idx, unsigned long tOnMs, unsigned long tOffMs);

private:
  bool _last[8] = {true,true,true,true,true,true,true,true}; // active-low buttons, idle high
  LedMode _ledModes[8] = {LedMode::Off,LedMode::Off,LedMode::Off,LedMode::Off,LedMode::Off,LedMode::Off,LedMode::Off,LedMode::Off};
  uint8_t _ledDuty[8] = {0,0,0,0,0,0,0,0}; // logical 0-255 (inverted when written)
  unsigned long _ledOnMs[8] = {0};
  unsigned long _ledOffMs[8] = {0};
  bool _ledPhaseOn[8] = {false,false,false,false,false,false,false,false};
  unsigned long _ledLastToggleMs[8] = {0};

  void updateLeds();
};
