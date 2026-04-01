#include "App.h"
#include "AnimComProtocol.h"
#include "BoardPins.h"
#include "Log.h"
#include "Utils.h"
#include <Watchdog_t4.h>
#include <cstdarg>
#include <cstdio>
#include <cstring>

App g_app;
WDT_T4<WDT1> wdt;

namespace {
static constexpr uint32_t kBootScreenMs = 500;
static constexpr uint32_t kControlKeepaliveMs = 200;
static constexpr uint32_t kManualCommandRepeatMs = 50;

const StaticEndpointDef *findStaticEndpoint(uint8_t stationId) {
  for (uint8_t i = 0; i < kStaticEndpointCount; i++) {
    if (kStaticEndpoints[i].stationId == stationId) {
      return &kStaticEndpoints[i];
    }
  }
  return nullptr;
}

bool parseManualMode(const char *text, ControllerMode &mode) {
  if (!text) {
    return false;
  }
  if (strcmp(text, "stop") == 0) {
    mode = ControllerMode::Stop;
    return true;
  }
  if (strcmp(text, "run") == 0 || strcmp(text, "auto") == 0) {
    mode = ControllerMode::Run;
    return true;
  }
  if (strcmp(text, "hold") == 0 || strcmp(text, "manual_hold") == 0) {
    mode = ControllerMode::ManualHold;
    return true;
  }
  if (strcmp(text, "manual") == 0 || strcmp(text, "manual_run") == 0) {
    mode = ControllerMode::ManualRun;
    return true;
  }
  return false;
}

void rebootNow() {
  Serial.flush();
  delay(50);
#if defined(NVIC_SystemReset)
  NVIC_SystemReset();
#else
  SCB_AIRCR = 0x05FA0004;
  while (true) { }
#endif
}

void handleConsoleCommand(const CommandMsg &msg) {
  g_app.handleConsoleCommand(msg);
}
}

void App::begin() {
  _console.setDispatchCommand(::handleConsoleCommand);
  _console.begin();

  _buttons.begin();
  _leds.begin();
  _analogs.begin();
  _enc.begin(PIN_ENC_A, PIN_ENC_B);
  _ui.begin();
  _animcom.begin(Serial2, PIN_ANIMCOM_DE);

  for (uint8_t i = 0; i < kStaticItemCount; i++) {
    _manualValues[i] = kStaticItems[i].defaultValue;
  }

  _sdReady = _sd.begin();
  _bootStartMs = millis();
  updateModeLeds();
  setStatusLine("SD:%s", _sdReady ? "OK" : "ERR");
  sendControlKeepalive(true);
}

void App::loop() {
  wdt.feed();

  _console.poll();
  const ButtonState buttonState = _buttons.poll();
  (void)_analogs.read();
  const int32_t encoderDelta = _enc.consumeDelta();

  if (buttonState.justPressed(Button::BUTTON_RED)) {
    setControllerMode(ControllerMode::Stop, true);
  } else if (buttonState.justPressed(Button::BUTTON_GREEN)) {
    setControllerMode(ControllerMode::Run, true);
  } else if (buttonState.justPressed(Button::BUTTON_YELLOW)) {
    if (_mode == ControllerMode::ManualRun) {
      setControllerMode(ControllerMode::ManualHold, true);
    } else if (_mode == ControllerMode::ManualHold) {
      setControllerMode(ControllerMode::ManualRun, true);
    } else {
      setControllerMode(ControllerMode::ManualHold, true);
    }
  }

  if (_mode == ControllerMode::Run) {
    handleRunButtons(buttonState);
    if (encoderDelta != 0) {
      handleRunEncoder(encoderDelta);
    }
  } else if (_mode == ControllerMode::ManualHold) {
    handleManualHoldButtons(buttonState);
  } else if (_mode == ControllerMode::ManualRun) {
    handleManualRunButtons(buttonState);
    if (encoderDelta != 0) {
      handleManualRunEncoder(encoderDelta);
    }
  }

  _leds.update();
  sendControlKeepalive(false);
  if (_mode == ControllerMode::ManualRun) {
    sendManualCommand(false);
  }
  updateUiModel();
  _ui.render(_model);
}

void App::updateModeLeds() {
  _leds.setMode(LED::LED_RED_BUTTON, (_mode == ControllerMode::Stop) ? LedMode::On : LedMode::Off);
  _leds.setMode(LED::LED_GREEN_BUTTON, (_mode == ControllerMode::Run) ? LedMode::On : LedMode::Off);
  _leds.setMode(LED::LED_YELLOW_BUTTON, (_mode == ControllerMode::ManualRun) ? LedMode::On : LedMode::Off);
}

void App::setControllerMode(ControllerMode mode, bool forceSend) {
  _mode = mode;
  updateModeLeds();

  if (_mode == ControllerMode::Run) {
    setStatusLine("RUN %u @ %u%%", (unsigned)_runPattern, (unsigned)_runSpeedScale);
  } else if (_mode == ControllerMode::ManualHold) {
    setStatusLine("MANUAL HOLD");
  } else if (_mode == ControllerMode::ManualRun) {
    setStatusLine("MANUAL RUN");
  } else {
    setStatusLine("STOP");
  }

  sendControlKeepalive(true);
  if (_mode == ControllerMode::ManualRun) {
    _lastManualSentItem = 0xFF;
    _lastManualSentValue = INT32_MIN;
    sendManualCommand(forceSend);
  }
}

void App::sendControlKeepalive(bool forceSend) {
  const uint32_t now = millis();
  if (!forceSend && (now - _lastControlSendMs) < kControlKeepaliveMs) {
    return;
  }

  uint8_t state = ANIMCOM_STATE_STOP;
  uint8_t pattern = 0;
  uint8_t speedScale = 100;
  if (_mode == ControllerMode::Run) {
    state = ANIMCOM_STATE_RUN_AUTO;
    pattern = _runPattern;
    speedScale = _runSpeedScale;
  } else if (_mode == ControllerMode::ManualHold || _mode == ControllerMode::ManualRun) {
    state = ANIMCOM_STATE_MANUAL;
  }

  _animcom.sendControlState(ANIMCOM_STATION_BCAST, state, pattern, speedScale);
  _lastControlSendMs = now;
}

void App::handleRunButtons(const ButtonState &buttonState) {
  if (buttonState.justPressed(Button::BUTTON_UP) && _pendingRunPattern < kRunPatternMax) {
    _pendingRunPattern++;
    _runPatternPending = (_pendingRunPattern != _runPattern);
    setStatusLine("PENDING ANIM %u", (unsigned)_pendingRunPattern);
  } else if (buttonState.justPressed(Button::BUTTON_DOWN) && _pendingRunPattern > kRunPatternMin) {
    _pendingRunPattern--;
    _runPatternPending = (_pendingRunPattern != _runPattern);
    setStatusLine("PENDING ANIM %u", (unsigned)_pendingRunPattern);
  } else if (buttonState.justPressed(Button::BUTTON_OK) && _runPatternPending) {
    _runPattern = _pendingRunPattern;
    _runPatternPending = false;
    setStatusLine("RUN %u @ %u%%", (unsigned)_runPattern, (unsigned)_runSpeedScale);
    sendControlKeepalive(true);
  }
}

void App::handleRunEncoder(int32_t encoderDelta) {
  _runSpeedScale = clampU8(static_cast<int32_t>(_runSpeedScale) + encoderDelta, 0, 200);
  setStatusLine("RUN %u @ %u%%", (unsigned)_runPattern, (unsigned)_runSpeedScale);
  sendControlKeepalive(true);
}

void App::handleManualHoldButtons(const ButtonState &buttonState) {
  if (buttonState.justPressed(Button::BUTTON_UP)) {
    moveManualSelection(0, -1);
  } else if (buttonState.justPressed(Button::BUTTON_DOWN)) {
    moveManualSelection(0, 1);
  } else if (buttonState.justPressed(Button::BUTTON_LEFT)) {
    moveManualSelection(-1, 0);
  } else if (buttonState.justPressed(Button::BUTTON_RIGHT)) {
    moveManualSelection(1, 0);
  }
}

void App::handleManualRunButtons(const ButtonState &buttonState) {
  if (buttonState.justPressed(Button::BUTTON_OK)) {
    triggerSelectedAudio();
  }
}

void App::handleManualRunEncoder(int32_t encoderDelta) {
  const StaticItemDef &item = selectedItem();
  int32_t &value = _manualValues[_selectedManualItem];
  value = clampI32(static_cast<int64_t>(value) + (static_cast<int64_t>(encoderDelta) * item.step),
                   item.minValue,
                   item.maxValue);
  sendManualCommand(false);
}

void App::moveManualSelection(int8_t dx, int8_t dy) {
  const uint8_t rows = (kStaticItemCount + kManualGridColumns - 1) / kManualGridColumns;
  int row = _selectedManualItem / kManualGridColumns;
  int col = _selectedManualItem % kManualGridColumns;

  row = (row + rows + dy) % rows;
  col = (col + kManualGridColumns + dx) % kManualGridColumns;

  int next = row * kManualGridColumns + col;
  while (next >= kStaticItemCount) {
    if (dx != 0) {
      col = (col + kManualGridColumns - 1) % kManualGridColumns;
    } else {
      row = (row + rows - 1) % rows;
    }
    next = row * kManualGridColumns + col;
  }

  _selectedManualItem = static_cast<uint8_t>(next);
  setStatusLine("%s", selectedItem().name);
}

const StaticItemDef &App::selectedItem() const {
  return kStaticItems[_selectedManualItem];
}

void App::sendManualCommand(bool forceSend) {
  if (_mode != ControllerMode::ManualRun) {
    return;
  }

  const uint32_t now = millis();
  const StaticItemDef &item = selectedItem();
  const int32_t value = _manualValues[_selectedManualItem];
  const bool sameCommand = (_lastManualSentItem == _selectedManualItem &&
                            _lastManualSentValue == value);
  if (!forceSend && sameCommand && (now - _lastManualSendMs) < kManualCommandRepeatMs) {
    return;
  }

  if (item.parameter == ManualParameter::AudioTrigger) {
    _lastManualSentItem = _selectedManualItem;
    _lastManualSentValue = value;
    _lastManualSendMs = now;
    return;
  }

  uint8_t cmdType = ANIMCOM_CMD_SPEED_PERCENT;
  if (item.parameter == ManualParameter::VelocityDegPerSec) {
    cmdType = ANIMCOM_CMD_SPEED_DEG_PER_SEC;
  } else if (item.parameter == ManualParameter::PositionDeg) {
    cmdType = ANIMCOM_CMD_POSITION_DEG;
  }

  const StaticEndpointDef *endpoint = findStaticEndpoint(item.stationId);
  if (endpoint && endpoint->type == EndpointType::AnimComOctal &&
      item.parameter == ManualParameter::SpeedPercent) {
    int8_t speeds[ANIMCOM_BULK_MOTORS] = {};
    const int8_t pct = static_cast<int8_t>(clampI32(value, -100, 100));
    for (uint8_t i = 0; i < ANIMCOM_BULK_MOTORS; i++) {
      speeds[i] = pct;
    }
    _animcom.sendManualBulk(item.stationId, speeds);
  } else {
    _animcom.sendManualSingle(item.stationId, item.channelIndex, cmdType, value);
  }
  _lastManualSentItem = _selectedManualItem;
  _lastManualSentValue = value;
  _lastManualSendMs = now;
}

void App::triggerSelectedAudio() {
  if (_mode != ControllerMode::ManualRun) {
    return;
  }
  const StaticItemDef &item = selectedItem();
  if (item.parameter != ManualParameter::AudioTrigger) {
    return;
  }
  const uint8_t track = clampU8(_manualValues[_selectedManualItem], 1, 255);
  _animcom.sendTriggerEffect(item.stationId, ANIMCOM_EFFECT_AUDIO_PLAY, track, 0);
  setStatusLine("AUDIO %u", (unsigned)track);
}

void App::updateUiModel() {
  _model.bootScreen = (millis() - _bootStartMs) < kBootScreenMs;
  _model.mode = _mode;
  _model.runSpeedScale = _runSpeedScale;
  _model.runPattern = _runPattern;
  _model.pendingRunPattern = _pendingRunPattern;
  _model.runPatternPending = _runPatternPending;
  _model.selectedManualItem = _selectedManualItem;
  _model.manualParameter = selectedItem().parameter;
  _model.manualValue = _manualValues[_selectedManualItem];
  _model.manualMin = selectedItem().minValue;
  _model.manualMax = selectedItem().maxValue;
  _model.txFrames = _animcom.getTxFrames();
  snprintf(_model.statusLine, sizeof(_model.statusLine), "%s", _statusLine);
}

void App::handleConsoleCommand(const CommandMsg &msg) {
  if (strcmp(msg.cmd, "help") == 0) {
    Serial.println("Commands:");
    Serial.println("  help");
    Serial.println("  sd dir [path]");
    Serial.println("  sd read <path>");
    Serial.println("  sd test");
    Serial.println("  mode <stop|run|hold|manual>");
    Serial.println("  reboot");
    return;
  }

  if (strcmp(msg.cmd, "sd") == 0) {
    if (!_sdReady) {
      statusMessage("SD: not ready");
      return;
    }
    if (msg.argc == 0) {
      statusMessage("sd dir [path] | sd read <path> | sd test");
      return;
    }
    if (strcmp(msg.argv[0], "dir") == 0) {
      const char *path = (msg.argc > 1) ? msg.argv[1] : "/";
      if (!_sd.listDir(path, Serial)) {
        statusMessage("SD: dir failed");
      }
      return;
    }
    if (strcmp(msg.argv[0], "read") == 0) {
      if (msg.argc < 2 || !_sd.readFile(msg.argv[1], Serial)) {
        statusMessage("SD: read failed");
      }
      return;
    }
    if (strcmp(msg.argv[0], "test") == 0) {
      actionSdTest();
      return;
    }
  }

  if (strcmp(msg.cmd, "mode") == 0) {
    if (msg.argc == 0) {
      statusMessage("mode stop|run|hold|manual");
      return;
    }
    ControllerMode mode = ControllerMode::Stop;
    if (!parseManualMode(msg.argv[0], mode)) {
      statusMessage("MODE: invalid");
      return;
    }
    setControllerMode(mode, true);
    return;
  }

  if (strcmp(msg.cmd, "reboot") == 0) {
    actionReboot();
    return;
  }

  statusMessage("Unknown command. Type 'help'.");
}

void App::setStatusLine(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vsnprintf(_statusLine, sizeof(_statusLine), fmt, args);
  va_end(args);
}

void App::statusMessage(const char *fmt, ...) {
  char buf[96] = {};
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  Serial.println(buf);
  setStatusLine("%s", buf);
}

void App::actionSdTest() {
  const bool ok = _sd.testReadWrite(Serial);
  setStatusLine("SD TEST: %s", ok ? "OK" : "FAIL");
}

void App::actionReboot() {
  setStatusLine("REBOOTING");
  rebootNow();
}

void watchdogCallback() {
  Serial.println("WATCHDOG TRIGGERED - SYSTEM RESET");
}

void setup() {
  logInit(115200);
  WDT_timings_t config;
  config.timeout = 5;
  config.trigger = 4;
  config.callback = watchdogCallback;
  wdt.begin(config);
  g_app.begin();
}

void loop() {
  g_app.loop();
}

