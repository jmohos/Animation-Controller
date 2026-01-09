#pragma once
#include <Arduino.h>

class Timebase {
public:
  /**
   * Description: Reset the timebase origin to the current time.
   * Inputs: None.
   * Outputs: Updates the internal reference timestamp.
   */
  void reset() { _t0Us = micros(); }

  /**
   * Description: Get elapsed microseconds since the last reset.
   * Inputs: None.
   * Outputs: Returns elapsed microseconds.
   */
  uint32_t nowUs() const { return (uint32_t)(micros() - _t0Us); }

  /**
   * Description: Get elapsed milliseconds since the last reset.
   * Inputs: None.
   * Outputs: Returns elapsed milliseconds.
   */
  uint32_t nowMs() const { return nowUs() / 1000u; }

private:
  uint32_t _t0Us = 0;
};
