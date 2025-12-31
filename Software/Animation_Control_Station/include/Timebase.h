#pragma once
#include <Arduino.h>

class Timebase {
public:
  void reset() { _t0_us = micros(); }
  uint32_t nowUs() const { return (uint32_t)(micros() - _t0_us); }
  uint32_t nowMs() const { return nowUs() / 1000u; }

private:
  uint32_t _t0_us = 0;
};
