#include "Input.h"
#include "BoardPins.h"
#include "Log.h"

#include <Wire.h>
#include <SparkFunSX1509.h>

static SX1509 g_sx;

static float readPotNorm(uint8_t pin) {
  const int v = analogRead(pin);
  const float maxv = 1023.0f;
  float x = (float)v / maxv;
  if (x < 0) x = 0;
  if (x > 1) x = 1;
  return x;
}

static bool readBtn(uint8_t sxPin) {
  // buttons short to ground -> pressed = LOW
  return (g_sx.digitalRead(sxPin) == LOW);
}

static void initButtons() {
  g_sx.debounceTime(32); // ~32ms debounce
  for (uint8_t i = 0; i < 8; i++) {
    g_sx.pinMode(i, INPUT_PULLUP);
    g_sx.debouncePin(i);
    g_sx.enableInterrupt(i, CHANGE);
  }
}

static void initLeds() {
  for (uint8_t pin = 8; pin < 16; pin++) {
    g_sx.ledDriverInit(pin); // linear, default freq
    // Drive inverted: ULN2803A sinks when input is high, so keep off at max.
    g_sx.analogWrite(pin, 255);
  }
}

static inline uint8_t ledPinForIdx(uint8_t idx) {
  return 8 + idx; // LEDs map to SX1509 pins 8..15
}

bool Input::begin() {
  Wire.begin();
  Wire.setClock(400000);

  pinMode(PIN_SX1509_INT, INPUT_PULLUP);

  const uint8_t addr = 0x3E; // SparkFun default, depends on ADDR pins
  if (!g_sx.begin(addr, Wire)) {
    LOGI("SX1509 begin failed at 0x%02X", addr);
    return false;
  }

  initButtons();
  initLeds();

  analogReadResolution(10);

  LOGI("SX1509 initialized");
  return true;
}

InputState Input::poll() {
  InputState s;

  // Button order must match _last[] order (8 buttons)
  std::array<bool, BUTTON_COUNT> now = {
    readBtn((uint8_t)SXPin::Mode_Button),
    readBtn((uint8_t)SXPin::Up_Button),
    readBtn((uint8_t)SXPin::Down_Button),
    readBtn((uint8_t)SXPin::Left_Button),
    readBtn((uint8_t)SXPin::Right_Button),
    readBtn((uint8_t)SXPin::OK_Button),
    readBtn((uint8_t)SXPin::Play_Button),
    readBtn((uint8_t)SXPin::Spare_Button),
  };

  // If interrupt is asserted, clear it
  if (digitalRead(PIN_SX1509_INT) == LOW) {
    g_sx.interruptSource(); // clear; states already read into now[]
  }

  for (uint8_t i = 0; i < BUTTON_COUNT; i++) {
    s.isPressed[i]      = now[i];
    s.isJustPressed[i]  = (now[i] && !_last[i]);
    s.isJustReleased[i] = (_last[i] && !now[i]);
    _last[i] = now[i];
  }

  // Update LED blink states
  updateLeds();

  // Pots
  s.potSpeedNorm = readPotNorm(PIN_POT_SPEED);
  s.potAccelNorm = readPotNorm(PIN_POT_ACCEL);

  // encoderDelta is filled by App from EncoderJog (not here)
  s.encoderDelta = 0;

  return s;
}

void Input::updateLeds() {
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

void Input::setPlayLed(bool on) {
  setLedSteady(0, on ? 255 : 0); // use LED0 for play indicator
}

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

LedMode Input::getLedMode(uint8_t idx) const {
  if (idx >= 8) return LedMode::Off;
  return _ledModes[idx];
}

void Input::setLedSteady(uint8_t idx, uint8_t duty) {
  if (idx >= 8) return;
  _ledModes[idx] = (duty == 0) ? LedMode::Off : LedMode::On;
  _ledDuty[idx] = duty;
  _ledOnMs[idx] = _ledOffMs[idx] = 0;
  _ledLastToggleMs[idx] = millis();
  const uint8_t pin = ledPinForIdx(idx);
  g_sx.analogWrite(pin, 255 - duty);
}

void Input::setLedBlink(uint8_t idx, unsigned long tOnMs, unsigned long tOffMs) {
  setLedMode(idx, LedMode::Blink, tOnMs, tOffMs);
}
