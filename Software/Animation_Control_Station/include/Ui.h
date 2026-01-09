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
  elapsedMillis _timeSinceLastRender;
  bool _ready = false;
  static constexpr unsigned long RENDER_PERIOD_MSEC = 100;

public:
  /**
   * Description: Initialize the display and draw the startup screen.
   * Inputs: None.
   * Outputs: Sets up display state and draws initial UI.
   */
  void begin();

  /**
   * Description: Render the UI based on the current model snapshot.
   * Inputs:
   * - model: UI model data to display.
   * Outputs: Updates the display if the render period has elapsed.
   */
  void render(const UiModel& model);
};
