#pragma once
#include <climits>
#include "AnalogInputs.h"
#include "AnimComMaster.h"
#include "Buttons.h"
#include "Console.h"
#include "ControllerTypes.h"
#include "EncoderJog.h"
#include "Leds.h"
#include "SdCard.h"
#include "StaticConfig.h"
#include "Ui.h"

class App {
public:
  void begin();
  void loop();
  void handleConsoleCommand(const CommandMsg &msg);
  void actionSdTest();
  void actionReboot();

private:
  void updateModeLeds();
  void setControllerMode(ControllerMode mode, bool forceSend = false);
  void sendControlKeepalive(bool forceSend = false);
  void handleRunButtons(const ButtonState &buttonState);
  void handleRunEncoder(int32_t encoderDelta);
  void handleManualHoldButtons(const ButtonState &buttonState);
  void handleManualRunButtons(const ButtonState &buttonState);
  void handleManualRunEncoder(int32_t encoderDelta);
  void moveManualSelection(int8_t dx, int8_t dy);
  const StaticItemDef &selectedItem() const;
  void sendManualCommand(bool forceSend = false);
  void triggerSelectedAudio();
  void updateUiModel();
  void setStatusLine(const char *fmt, ...);
  void statusMessage(const char *fmt, ...);

  Console _console;
  Buttons _buttons;
  Leds _leds;
  AnalogInputs _analogs;
  Ui _ui;
  EncoderJog _enc;
  SdCardManager _sd;
  AnimComMaster _animcom;
  UiModel _model;
  ControllerMode _mode = ControllerMode::Stop;
  uint8_t _selectedManualItem = 0;
  int32_t _manualValues[kStaticItemCount] = {};
  uint8_t _runSpeedScale = 100;
  uint8_t _runPattern = kRunPatternDefault;
  uint8_t _pendingRunPattern = kRunPatternDefault;
  bool _runPatternPending = false;
  uint8_t _lastManualSentItem = 0xFF;
  int32_t _lastManualSentValue = INT32_MIN;
  bool _sdReady = false;
  char _statusLine[32] = {};
  uint32_t _bootStartMs = 0;
  uint32_t _lastControlSendMs = 0;
  uint32_t _lastManualSendMs = 0;
};
