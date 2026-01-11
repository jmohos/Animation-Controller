#include "Leds.h"
#include "Sx1509Bus.h"

static constexpr uint8_t kLedPinBase = 8;

/**
 * Description: Configure SX1509 pins for LED driver output.
 * Inputs: None.
 * Outputs: Initializes LED drivers and sets them off.
 */
static void initLeds() {
  for (uint8_t idx = 0; idx < LED_COUNT; idx++) {
    const uint8_t pin = kLedPinBase + idx;
    sx1509LedDriverInit(pin); // linear, default freq
    // Drive inverted: ULN2803A sinks when input is high, so keep off at max.
    sx1509AnalogWrite(pin, 255);
  }
}

/**
 * Description: Map a logical LED index to the SX1509 pin number.
 * Inputs:
 * - idx: LED index [0..7].
 * Outputs: Returns the SX1509 pin number.
 */
static inline uint8_t ledPinForIdx(uint8_t idx) {
  return kLedPinBase + idx; // LEDs map to SX1509 pins 8..15
}

/**
 * Description: Convert an LED enum value into a logical LED index.
 * Inputs:
 * - led: LED enumeration value.
 * Outputs: Returns LED index [0..7].
 */
static inline uint8_t ledIndex(LED led) {
  return static_cast<uint8_t>(led);
}

/**
 * Description: Initialize LED drivers.
 * Inputs: None.
 * Outputs: Returns true when initialization succeeds.
 */
bool Leds::begin() {
  if (!sx1509EnsureReady()) {
    return false;
  }
  initLeds();
  return true;
}

/**
 * Description: Update LED outputs and blink phases.
 * Inputs: None.
 * Outputs: Writes LED states to the SX1509.
 */
void Leds::update() {
  if (!sx1509Ready()) {
    return;
  }
  const unsigned long nowMs = millis();
  for (uint8_t i = 0; i < LED_COUNT; i++) {
    const uint8_t pin = ledPinForIdx(i);
    switch (_ledModes[i]) {
      case LedMode::Off:
        sx1509AnalogWrite(pin, 255); // inverted: 255 = off
        _ledLastToggleMs[i] = nowMs;
        _ledPhaseOn[i] = false;
        break;
      case LedMode::On:
        sx1509AnalogWrite(pin, 255 - _ledDuty[i]);   // inverted: duty stored logical
        _ledLastToggleMs[i] = nowMs;
        _ledPhaseOn[i] = true;
        break;
      case LedMode::Blink: {
        const unsigned long onMs  = (_ledOnMs[i]  == 0) ? 1 : _ledOnMs[i];
        const unsigned long offMs = (_ledOffMs[i] == 0) ? 1 : _ledOffMs[i];
        if (_ledLastToggleMs[i] == 0) {
          // initialize phase
          _ledPhaseOn[i] = true;
          _ledLastToggleMs[i] = nowMs;
          sx1509AnalogWrite(pin, 0);
        } else {
          const unsigned long interval = _ledPhaseOn[i] ? onMs : offMs;
          if (nowMs - _ledLastToggleMs[i] >= interval) {
            _ledPhaseOn[i] = !_ledPhaseOn[i];
            _ledLastToggleMs[i] = nowMs;
            sx1509AnalogWrite(pin, _ledPhaseOn[i] ? 0 : 255);
          }
        }
        break;
      }
    }
  }
}

/**
 * Description: Configure an LED mode and timing.
 * Inputs:
 * - led: LED to configure.
 * - mode: desired LED mode.
 * - tOnMs: blink on-time (ms).
 * - tOffMs: blink off-time (ms).
 * Outputs: Updates LED mode, duty, and timing.
 */
void Leds::setMode(LED led, LedMode mode, unsigned long tOnMs, unsigned long tOffMs) {
  const uint8_t idx = ledIndex(led);
  if (idx >= LED_COUNT) return;
  _ledModes[idx] = mode;
  if (mode == LedMode::Blink) {
    _ledOnMs[idx] = tOnMs;
    _ledOffMs[idx] = tOffMs;
    _ledLastToggleMs[idx] = 0; // re-init phase on next update
    _ledDuty[idx] = 255;
  } else {
    _ledOnMs[idx] = _ledOffMs[idx] = 0;
    _ledLastToggleMs[idx] = millis();
    _ledDuty[idx] = (mode == LedMode::On) ? 255 : 0;
  }
}

/**
 * Description: Get the current LED mode.
 * Inputs:
 * - led: LED to query.
 * Outputs: Returns the LED mode.
 */
LedMode Leds::getMode(LED led) const {
  const uint8_t idx = ledIndex(led);
  if (idx >= LED_COUNT) return LedMode::Off;
  return _ledModes[idx];
}

/**
 * Description: Set a steady LED duty cycle.
 * Inputs:
 * - led: LED to configure.
 * - duty: duty cycle (0-255).
 * Outputs: Updates LED duty and output.
 */
void Leds::setSteady(LED led, uint8_t duty) {
  const uint8_t idx = ledIndex(led);
  if (idx >= LED_COUNT) return;
  _ledModes[idx] = (duty == 0) ? LedMode::Off : LedMode::On;
  _ledDuty[idx] = duty;
  _ledOnMs[idx] = _ledOffMs[idx] = 0;
  _ledLastToggleMs[idx] = millis();
  if (sx1509Ready()) {
    const uint8_t pin = ledPinForIdx(idx);
    sx1509AnalogWrite(pin, 255 - duty);
  }
}

/**
 * Description: Configure a blinking LED mode.
 * Inputs:
 * - led: LED to configure.
 * - tOnMs: on-time in milliseconds.
 * - tOffMs: off-time in milliseconds.
 * Outputs: Updates LED mode and timing.
 */
void Leds::setBlink(LED led, unsigned long tOnMs, unsigned long tOffMs) {
  setMode(led, LedMode::Blink, tOnMs, tOffMs);
}
