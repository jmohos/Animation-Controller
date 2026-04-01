#pragma once
#include <Arduino.h>
#include <elapsedMillis.h>
#include "ControllerTypes.h"

struct UiModel {
  bool bootScreen = true;
  ControllerMode mode = ControllerMode::Stop;
  uint8_t runSpeedScale = 100;
  uint8_t runPattern = 1;
  uint8_t pendingRunPattern = 1;
  bool runPatternPending = false;
  uint8_t selectedManualItem = 0;
  ManualParameter manualParameter = ManualParameter::SpeedPercent;
  int32_t manualValue = 0;
  int32_t manualMin = 0;
  int32_t manualMax = 0;
  uint32_t txFrames = 0;
  char statusLine[32] = {};
};

class Ui {
  elapsedMillis _timeSinceLastRender;
  bool _ready = false;
  static constexpr unsigned long RENDER_PERIOD_MSEC = 100;

public:
  void begin();
  void render(const UiModel& model);
};
