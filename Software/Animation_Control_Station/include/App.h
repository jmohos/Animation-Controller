#pragma once
#include "Input.h"
#include "Ui.h"
#include "ShowEngine.h"
#include "EncoderJog.h"
#include "Rs422Ports.h"

class App {
public:
  void begin();
  void loop();

private:
  Input _input;
  Ui _ui;
  ShowEngine _show;
  EncoderJog _enc;
  Rs422Ports _rs422;

  UiModel _model;
};
