#pragma once
#include "Console.h"
#include "Buttons.h"
#include "Leds.h"
#include "AnalogInputs.h"
#include "Ui.h"
#include "ShowEngine.h"
#include "EncoderJog.h"
#include "Rs422Ports.h"
#include "ConfigStore.h"
#include "SdCard.h"
#include "RoboClawBus.h"
#include "CanBus.h"
#include "SequencePlayer.h"

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

  /**
   * Description: Handle console commands from USB serial.
   * Inputs:
   * - msg: parsed command message.
   * Outputs: Executes matching command handlers.
   */
  void handleConsoleCommand(const CommandMsg &msg);

  /**
   * Description: Menu action handlers used by menu callbacks.
   * Inputs: None.
   * Outputs: Executes the selected action.
   */
  void actionSaveConfig();
  void actionResetConfig();
  void actionFactoryReset();
  void actionOpenEdit();
  void actionSdTest();
  void actionReboot();

private:
  void pollRoboClaws();
  void stopRoboClaws();
  void saveAnimationEdits();
  bool getEditEvent(SequenceEvent &event, uint16_t &ordinal, uint16_t &count);
  bool selectFirstEditEvent(uint8_t endpointId);
  bool selectNeighborEditEvent(uint8_t endpointId, size_t startIndex);
  void adjustEndpointField(int32_t delta);
  void setStatusLine(const char *fmt, ...);
  void setStatusConfigSave(bool epOk);
  void setStatusConfigReset(bool epOk);
  void setStatusSdTest(bool ok);
  void setStatusRebooting();
  void statusMessage(const char *fmt, ...);

  Console _console;
  Buttons _buttons;
  Leds _leds;
  AnalogInputs _analogs;
  Ui _ui;
  ShowEngine _show;
  EncoderJog _enc;
  Rs422Ports _rs422;
  RoboClawBus _roboclaw;
  CanBus _can;
  ConfigStore _configStore;
  AppConfig _config;
  SdCardManager _sd;
  SequencePlayer _sequence;
  UiModel _model;
  UiScreen _screen = UiScreen::Boot;
  UiScreen _screenBeforeMenu = UiScreen::Manual;
  UiScreen _lastRunScreen = UiScreen::Manual;
  uint8_t _menuIndex = 0;
  uint8_t _settingsIndex = 0;
  uint8_t _diagnosticsIndex = 0;
  uint8_t _endpointConfigIndex = 0;
  uint8_t _endpointConfigField = 0;
  bool _endpointConfigEditing = false;
  size_t _editEventIndex = 0;
  uint8_t _editField = 0;
  int32_t _editPosTickAccum = 0;
  bool _sdReady = false;
  bool _configLoaded = false;
  bool _configFromEndpoints = false;
  bool _sequenceLoaded = false;
  char _statusLine[32] = {};
  uint32_t _bootStartMs = 0;
  static constexpr uint32_t kRcPollPeriodMs = 100;
  RoboClawStatus _rcStatusByEndpoint[MAX_ENDPOINTS] = {};
  bool _rcStatusByEndpointValid[MAX_ENDPOINTS] = {};
  uint8_t _rcPollIndex = 0;
  RoboClawStatus _rcStatus;
  bool _rcStatusValid = false;
  uint32_t _lastRcPollMs = 0;
};
