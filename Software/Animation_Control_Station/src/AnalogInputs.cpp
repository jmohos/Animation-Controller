#include "AnalogInputs.h"
#include "BoardPins.h"

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
 * Description: Initialize ADC settings.
 * Inputs: None.
 * Outputs: None.
 */
void AnalogInputs::begin() {
  analogReadResolution(12); // use highest supported resolution on Teensy 4.1
}

/**
 * Description: Read potentiometers into a snapshot.
 * Inputs: None.
 * Outputs: Returns the current analog state.
 */
AnalogState AnalogInputs::read() const {
  AnalogState state;
  state.potSpeedNorm = readPotNorm(PIN_POT_SPEED);
  state.potAccelNorm = readPotNorm(PIN_POT_ACCEL);
  return state;
}
