#pragma once
#include <Arduino.h>

struct AnalogState {
  float potSpeedNorm = 0.0f; // 0..1
  float potAccelNorm = 0.0f; // 0..1
};

class AnalogInputs {
public:
  /**
   * Description: Initialize ADC settings.
   * Inputs: None.
   * Outputs: None.
   */
  void begin();

  /**
   * Description: Read potentiometers into a snapshot.
   * Inputs: None.
   * Outputs: Returns the current analog state.
   */
  AnalogState read() const;
};
