#include "Input.h"
#include "BoardPins.h"
#include "Faults.h"
#include "Log.h"

#include <Wire.h>
#include <SparkFunSX1509.h>

static SX1509 g_sx;
static bool g_sxReady = false;

/**
 * Description: Read and normalize a potentiometer to 0.0-1.0 with end deadbands.
 * Inputs:
 * - pin: analog pin to sample.
 * Outputs: Returns normalized value with deadband and rescaled mid-range.
 */
static float readPotNorm(uint8_t pin) {
  const int adcValue = analogRead(pin);
  constexpr float maxValue = 4095.0f;    // 12-bit peak for Teensy 4.1 ADC
  constexpr float edgeDeadband = 0.01f; // 1% gap at each end to guarantee 0/100%

  float normalized = (float)adcValue / maxValue;
  if (normalized <= edgeDeadband) return 0.0f;
  if (normalized >= 1.0f - edgeDeadband) return 1.0f;

  // Re-scale the middle so mid-span remains linear after deadband removal.
  normalized = (normalized - edgeDeadband) / (1.0f - 2.0f * edgeDeadband);
  if (normalized < 0) normalized = 0;
  if (normalized > 1) normalized = 1;
  return normalized;
}

/**
 * Description: Read a debounced button state from the SX1509 expander.
 * Inputs:
 * - sxPin: SX1509 pin index.
 * Outputs: Returns true when the button is pressed.
 */
static bool readBtn(uint8_t sxPin) {
  // buttons short to ground -> pressed = LOW
  if (!g_sxReady) {
    return false;
  }
  return (g_sx.digitalRead(sxPin) == LOW);
}

/**
 * Description: Configure SX1509 pins for button input with debounce.
 * Inputs: None.
 * Outputs: Sets pin modes and debounce parameters.
 */
static void initButtons() {
  g_sx.debounceTime(32); // ~32ms debounce
  for (uint8_t i = 0; i < 8; i++) {
    g_sx.pinMode(i, INPUT_PULLUP);
    g_sx.debouncePin(i);
  }
}

/**
 * Description: Configure SX1509 pins for LED driver output.
 * Inputs: None.
 * Outputs: Initializes LED drivers and sets them off.
 */
static void initLeds() {
  for (uint8_t pin = 8; pin < 16; pin++) {
    g_sx.ledDriverInit(pin); // linear, default freq
    // Drive inverted: ULN2803A sinks when input is high, so keep off at max.
    g_sx.analogWrite(pin, 255);
  }
}

/**
 * Description: Map a logical LED index to the SX1509 pin number.
 * Inputs:
 * - idx: LED index [0..7].
 * Outputs: Returns the SX1509 pin number.
 */
static inline uint8_t ledPinForIdx(uint8_t idx) {
  return 8 + idx; // LEDs map to SX1509 pins 8..15
}

/**
 * Description: Initialize input devices and LED drivers.
 * Inputs: None.
 * Outputs: Returns true when initialization succeeds.
 */
bool Input::begin() {
  g_sxReady = false;
  Wire.begin();
  Wire.setClock(400000);

  pinMode(PIN_SX1509_INT, INPUT_PULLUP);

  const uint8_t deviceAddress = 0x3E; // SparkFun default, depends on ADDR pins
  if (!g_sx.begin(deviceAddress, Wire)) {
    LOGI("SX1509 begin failed at 0x%02X", deviceAddress);
    FAULT_SET(FAULT_IO_EXPANDER_FAULT);
    return false;
  }
  g_sxReady = true;

  initButtons();
  initLeds();

  analogReadResolution(12); // use highest supported resolution on Teensy 4.1

  LOGI("SX1509 initialized");
  return true;
}

/**
 * Description: Poll the inputs and produce a snapshot of current state.
 * Inputs: None.
 * Outputs: Returns the current InputState snapshot.
 */
InputState Input::poll() {
  InputState state;
  if (!g_sxReady) {
    state.potSpeedNorm = readPotNorm(PIN_POT_SPEED);
    state.potAccelNorm = readPotNorm(PIN_POT_ACCEL);
    return state;
  }

  // auto source = g_sx.interruptSource(true); // clear; states already read into buttonStates[]
  // if (source != 0x0000) {
  //   Serial.printf("BTN INT SOURCE: %X\n", source);
  // }

  // // If interrupt is asserted, clear it
  // if (digitalRead(PIN_SX1509_INT) == LOW) {
  //   for (uint8_t pin : {0,1,2,3,4,5,6,7}) {
  //     if (g_sx.checkInterrupt(pin)) {
  //       Serial.printf("BTN %d interrupt!\n", pin);
  //     }
  //   }
  // }

  // Button order must match _last[] order (8 buttons)
  std::array<bool, BUTTON_COUNT> buttonStates = {
    readBtn((uint8_t)SXPin::BUTTON_OK),
    readBtn((uint8_t)SXPin::BUTTON_DOWN),
    readBtn((uint8_t)SXPin::BUTTON_UP),
    readBtn((uint8_t)SXPin::BUTTON_LEFT),
    readBtn((uint8_t)SXPin::BUTTON_RIGHT),
    readBtn((uint8_t)SXPin::BUTTON_RED),
    readBtn((uint8_t)SXPin::BUTTON_YELLOW),
    readBtn((uint8_t)SXPin::BUTTON_GREEN),
  };



  for (uint8_t i = 0; i < BUTTON_COUNT; i++) {
    state.isPressed[i]      = buttonStates[i];
    state.isJustPressed[i]  = (buttonStates[i] && !_last[i]);
    state.isJustReleased[i] = (_last[i] && !buttonStates[i]);
    _last[i] = buttonStates[i];
  }

  // Update LED blink states
  updateLeds();

  // Pots
  state.potSpeedNorm = readPotNorm(PIN_POT_SPEED);
  state.potAccelNorm = readPotNorm(PIN_POT_ACCEL);

  // encoderDelta is filled by App from EncoderJog (not here)
  state.encoderDelta = 0;

  return state;
}

/**
 * Description: Update LED outputs and blink phases.
 * Inputs: None.
 * Outputs: Writes LED states to the SX1509.
 */
void Input::updateLeds() {
  if (!g_sxReady) {
    return;
  }
  const unsigned long nowMs = millis();
  for (uint8_t i = 0; i < 8; i++) {
    const uint8_t pin = ledPinForIdx(i);
    switch (_ledModes[i]) {
      case LedMode::Off:
        g_sx.analogWrite(pin, 255); // inverted: 255 = off
        _ledLastToggleMs[i] = nowMs;
        _ledPhaseOn[i] = false;
        break;
      case LedMode::On:
        g_sx.analogWrite(pin, 255 - _ledDuty[i]);   // inverted: duty stored logical
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
          g_sx.analogWrite(pin, 0);
        } else {
          const unsigned long interval = _ledPhaseOn[i] ? onMs : offMs;
          if (nowMs - _ledLastToggleMs[i] >= interval) {
            _ledPhaseOn[i] = !_ledPhaseOn[i];
            _ledLastToggleMs[i] = nowMs;
            g_sx.analogWrite(pin, _ledPhaseOn[i] ? 0 : 255);
          }
        }
        break;
      }
    }
  }
}

/**
 * Description: Turn the play LED on or off.
 * Inputs:
 * - on: true to illuminate, false to turn off.
 * Outputs: Updates LED output state.
 */
void Input::setPlayLed(bool on) {
  setLedSteady(0, on ? 255 : 0); // use LED0 for play indicator
}

/**
 * Description: Configure an LED mode and timing by index.
 * Inputs:
 * - idx: LED index [0..7].
 * - mode: desired LED mode.
 * - tOnMs: blink on-time (ms).
 * - tOffMs: blink off-time (ms).
 * Outputs: Updates LED mode, duty, and timing.
 */
void Input::setLedMode(uint8_t idx, LedMode mode, unsigned long tOnMs, unsigned long tOffMs) {
  if (idx >= 8) return;
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
 * Description: Get the current LED mode by index.
 * Inputs:
 * - idx: LED index [0..7].
 * Outputs: Returns the LED mode.
 */
LedMode Input::getLedMode(uint8_t idx) const {
  if (idx >= 8) return LedMode::Off;
  return _ledModes[idx];
}

/**
 * Description: Set a steady LED duty cycle.
 * Inputs:
 * - idx: LED index [0..7].
 * - duty: duty cycle (0-255).
 * Outputs: Updates LED duty and output.
 */
void Input::setLedSteady(uint8_t idx, uint8_t duty) {
  if (idx >= 8) return;
  _ledModes[idx] = (duty == 0) ? LedMode::Off : LedMode::On;
  _ledDuty[idx] = duty;
  _ledOnMs[idx] = _ledOffMs[idx] = 0;
  _ledLastToggleMs[idx] = millis();
  if (g_sxReady) {
    const uint8_t pin = ledPinForIdx(idx);
    g_sx.analogWrite(pin, 255 - duty);
  }
}

/**
 * Description: Configure a blinking LED mode.
 * Inputs:
 * - idx: LED index [0..7].
 * - tOnMs: on-time in milliseconds.
 * - tOffMs: off-time in milliseconds.
 * Outputs: Updates LED mode and timing.
 */
void Input::setLedBlink(uint8_t idx, unsigned long tOnMs, unsigned long tOffMs) {
  setLedMode(idx, LedMode::Blink, tOnMs, tOffMs);
}
