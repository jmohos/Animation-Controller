#pragma once
#include <Arduino.h>

struct UiModel {
  bool playing = false;
  uint32_t showTimeMs = 0;
  uint8_t selectedMotor = 0;
  float speedNorm = 0.0f;
  float accelNorm = 0.0f;
  int32_t jogPos = 0;
};

class Ui {
public:
  void begin();
  void render(const UiModel& m);
};
