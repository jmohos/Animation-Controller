#pragma once
#include <Arduino.h>

enum class ControllerMode : uint8_t {
  Stop = 0,
  Run = 1,
  ManualHold = 2,
  ManualRun = 3
};

enum class ManualParameter : uint8_t {
  SpeedPercent = 0,
  VelocityDegPerSec = 1,
  PositionDeg = 2,
  AudioTrigger = 3
};
