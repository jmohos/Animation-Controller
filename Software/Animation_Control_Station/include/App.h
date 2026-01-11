#pragma once
#include "Console.h"
#include "Buttons.h"
#include "Leds.h"
#include "AnalogInputs.h"
#include "Ui.h"
#include "ShowEngine.h"
#include "EncoderJog.h"
#include "Rs422Ports.h"

class App {
public:
  /**
   * Description: Initialize all subsystems for the application.
   * Inputs: None.
   * Outputs: Sets up input, UI, show engine, encoder, and RS422 ports.
   */
  void begin();

  /**
   * Description: Main application loop.
   * Inputs: None.
   * Outputs: Updates model state and renders UI each tick.
   */
  void loop();

private:
  Console _console;
  Buttons _buttons;
  Leds _leds;
  AnalogInputs _analogs;
  Ui _ui;
  ShowEngine _show;
  EncoderJog _enc;
  Rs422Ports _rs422;
  UiModel _model;
};
