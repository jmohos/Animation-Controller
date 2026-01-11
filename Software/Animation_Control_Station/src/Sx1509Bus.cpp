#include "Sx1509Bus.h"
#include "BoardPins.h"
#include "Faults.h"
#include "Log.h"
#include <Wire.h>
#include <SparkFunSX1509.h>

static SX1509 g_sx;
static bool g_sxReady = false;
static bool g_sxInitAttempted = false;

bool sx1509EnsureReady() {
  if (g_sxReady) {
    return true;
  }
  if (g_sxInitAttempted) {
    return false;
  }
  g_sxInitAttempted = true;

  Wire.begin();
  Wire.setClock(400000);
  pinMode(PIN_SX1509_INT, INPUT_PULLUP);

  const uint8_t deviceAddress = 0x3E; // SparkFun default, depends on ADDR pins
  if (!g_sx.begin(deviceAddress, Wire)) {
    LOGI("SX1509 begin failed at 0x%02X", deviceAddress);
    FAULT_SET(FAULT_IO_EXPANDER_FAULT);
    g_sxReady = false;
    return false;
  }

  g_sxReady = true;
  LOGI("SX1509 initialized");
  return true;
}

bool sx1509Ready() {
  return g_sxReady;
}

void sx1509DebounceTime(uint8_t timeMs) {
  if (!g_sxReady) return;
  g_sx.debounceTime(timeMs);
}

void sx1509PinMode(uint8_t pin, uint8_t mode) {
  if (!g_sxReady) return;
  g_sx.pinMode(pin, mode);
}

void sx1509DebouncePin(uint8_t pin) {
  if (!g_sxReady) return;
  g_sx.debouncePin(pin);
}

int sx1509DigitalRead(uint8_t pin) {
  if (!g_sxReady) return HIGH;
  return g_sx.digitalRead(pin);
}

void sx1509LedDriverInit(uint8_t pin) {
  if (!g_sxReady) return;
  g_sx.ledDriverInit(pin); // linear, default freq
}

void sx1509AnalogWrite(uint8_t pin, uint8_t value) {
  if (!g_sxReady) return;
  g_sx.analogWrite(pin, value);
}
