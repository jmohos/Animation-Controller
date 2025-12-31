#pragma once
#include <Arduino.h>
#include <elapsedMillis.h>


struct UiModel {
  bool playing = false;
  uint32_t showTimeMs = 0;
  uint8_t selectedMotor = 0;
  float speedNorm = 0.0f;
  float accelNorm = 0.0f;
  int32_t jogPos = 0;
};

class Ui {
  elapsedMillis _time_since_last_render;
  static constexpr unsigned long RENDER_PERIOD_MSEC = 100;

public:
  void begin();
  void render(const UiModel& m);
};
