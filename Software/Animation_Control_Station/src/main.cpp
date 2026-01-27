#include <Arduino.h>
#include "Log.h"
#include "App.h"
#include "BoardPins.h"
#include "Faults.h"
#include "MenuDefs.h"
#include "Utils.h"
#include "UnitConversion.h"
#include "MksServoProtocol.h"
#include <Watchdog_t4.h>
#include <cstring>
#include <cstdlib>
#include <cstdarg>
#include <cstdio>
#include <climits>

App g_app;
WDT_T4<WDT1> wdt;  // Watchdog timer instance

static constexpr uint32_t kCanBitrate = 500000;
static constexpr uint8_t kEndpointPortMin = 0;
static constexpr uint8_t kEndpointPortMax = RS422_PORT_COUNT;
static constexpr uint8_t kEndpointMotorMin = 1;
static constexpr uint8_t kEndpointMotorMax = 2;
static constexpr uint32_t kEndpointAddressMin = 0;
static constexpr uint32_t kEndpointAddressMax = 0x1FFFFFFF;
static constexpr int32_t kEndpointPositionStep = 1;
static constexpr uint32_t kEndpointVelocityStep = 1;
static constexpr uint32_t kEndpointAccelStep = 1;
static constexpr uint32_t kEndpointRateMax = 0xFFFFFFFFu;
static constexpr uint32_t kEditTimeMaxMs = 300000;
static constexpr int32_t kEditTimeStepMs = 100;
static constexpr int32_t kEditPositionStep = 1;
static constexpr int32_t kEditPositionTicksPerStep = 4;
static constexpr uint32_t kEditVelocityStep = 10;
static constexpr uint32_t kEditAccelStep = 10;
static constexpr uint32_t kEditNewEventOffsetMs = 1000;

enum class EditField : uint8_t {
  Time = 0,
  Position = 1,
  Velocity = 2,
  Accel = 3,
  Count = 4
};

static constexpr uint8_t kEditFieldCount = static_cast<uint8_t>(EditField::Count);

// Parsing and clamping functions moved to Utils.h/cpp

static bool parseBoolToken(const char *text, uint8_t &value) {
  if (!text) {
    return false;
  }
  if (strcmp(text, "on") == 0 || strcmp(text, "true") == 0 || strcmp(text, "enable") == 0) {
    value = 1;
    return true;
  }
  if (strcmp(text, "off") == 0 || strcmp(text, "false") == 0 || strcmp(text, "disable") == 0) {
    value = 0;
    return true;
  }
  uint32_t parsed = 0;
  if (!parseUint32(text, parsed)) {
    return false;
  }
  value = (parsed != 0) ? 1 : 0;
  return true;
}

static bool parseEndpointFieldName(const char *text, EndpointField &field) {
  if (!text) {
    return false;
  }
  if (strcmp(text, "enabled") == 0 || strcmp(text, "enable") == 0) {
    field = EndpointField::Enabled;
    return true;
  }
  if (strcmp(text, "type") == 0 || strcmp(text, "endpoint_type") == 0) {
    field = EndpointField::Type;
    return true;
  }
  if (strcmp(text, "serial") == 0 || strcmp(text, "port") == 0) {
    field = EndpointField::SerialPort;
    return true;
  }
  if (strcmp(text, "motor") == 0) {
    field = EndpointField::Motor;
    return true;
  }
  if (strcmp(text, "address") == 0 || strcmp(text, "addr") == 0) {
    field = EndpointField::Address;
    return true;
  }
  if (strcmp(text, "pos_min") == 0 || strcmp(text, "pmin") == 0 || strcmp(text, "position_min") == 0) {
    field = EndpointField::PositionMin;
    return true;
  }
  if (strcmp(text, "pos_max") == 0 || strcmp(text, "pmax") == 0 || strcmp(text, "position_max") == 0) {
    field = EndpointField::PositionMax;
    return true;
  }
  if (strcmp(text, "vmin") == 0 || strcmp(text, "min_velocity") == 0 || strcmp(text, "velocity_min") == 0) {
    field = EndpointField::VelocityMin;
    return true;
  }
  if (strcmp(text, "vmax") == 0 || strcmp(text, "max_velocity") == 0 || strcmp(text, "velocity_max") == 0) {
    field = EndpointField::VelocityMax;
    return true;
  }
  if (strcmp(text, "amin") == 0 || strcmp(text, "min_accel") == 0 || strcmp(text, "accel_min") == 0) {
    field = EndpointField::AccelMin;
    return true;
  }
  if (strcmp(text, "amax") == 0 || strcmp(text, "max_accel") == 0 || strcmp(text, "accel_max") == 0) {
    field = EndpointField::AccelMax;
    return true;
  }
  if (strcmp(text, "ppr") == 0 || strcmp(text, "pulses_per_rev") == 0 || strcmp(text, "pulses_per_revolution") == 0) {
    field = EndpointField::PulsesPerRev;
    return true;
  }
  if (strcmp(text, "home_offset") == 0 || strcmp(text, "homeoff") == 0) {
    field = EndpointField::HomeOffset;
    return true;
  }
  if (strcmp(text, "home_dir") == 0 || strcmp(text, "home_direction") == 0) {
    field = EndpointField::HomeDirection;
    return true;
  }
  if (strcmp(text, "limit") == 0 || strcmp(text, "has_limit") == 0 || strcmp(text, "has_limit_switch") == 0) {
    field = EndpointField::HasLimitSwitch;
    return true;
  }
  return false;
}

static bool resolveEndpoint(const AppConfig &config, uint8_t endpointIndex, const EndpointConfig *&ep, uint8_t &portIndex) {
  if (endpointIndex >= MAX_ENDPOINTS) {
    return false;
  }
  const EndpointConfig &candidate = config.endpoints[endpointIndex];
  if (!candidate.enabled) {
    return false;
  }
  if (candidate.type != EndpointType::RoboClaw) {
    return false;
  }
  if (candidate.serialPort < 1 || candidate.serialPort > MAX_RC_PORTS) {
    return false;
  }
  if (candidate.motor < 1 || candidate.motor > 2) {
    return false;
  }
  if (candidate.address > 0xFF) {
    return false;
  }
  ep = &candidate;
  portIndex = static_cast<uint8_t>(candidate.serialPort - 1);
  return true;
}

static bool usesCanBus(EndpointType type) {
  return (type == EndpointType::MksServo ||
          type == EndpointType::RevFrcCan ||
          type == EndpointType::JoeServoCan);
}

// MKS Servo protocol functions moved to MksServoProtocol.h/cpp

static bool sendMksServoPosition(CanBus &can, const EndpointConfig &endpoint, int32_t position,
                                 uint32_t velocity, uint32_t accel) {
  if (endpoint.address > 0x7FFu) {
    return false;
  }

  // Convert engineering units to device units if enabled
  int32_t devicePos = UnitConverter::degreesToPulses(static_cast<float>(position), endpoint);
  uint32_t deviceVel = UnitConverter::degPerSecToDeviceVelocity(static_cast<float>(velocity), endpoint);
  uint32_t deviceAccel = UnitConverter::degPerSec2ToDeviceAccel(static_cast<float>(accel), endpoint);

  // Clamp to device limits
  uint32_t speed = deviceVel;
  if (speed > MksServo::MAX_VELOCITY_RPM) {
    speed = MksServo::MAX_VELOCITY_RPM;
  }
  uint32_t acc = deviceAccel;
  if (acc > MksServo::MAX_ACCEL) {
    acc = MksServo::MAX_ACCEL;
  }

  const uint16_t canId = static_cast<uint16_t>(endpoint.address);

  // Pack the MKS Servo position command
  uint8_t data[8] = {};
  if (!MksServoProtocol::packPosition(canId, static_cast<uint16_t>(speed), static_cast<uint8_t>(acc), devicePos, data)) {
    return false;
  }
  Serial.printf("CAN TX ID: 0x%03X DATA: ", canId);
  for (int i = 0; i < 8; i++) {
    Serial.printf("%02X", data[i]);
    if (i < 7) {
      Serial.print(' ');
    }
  }
  Serial.print('\n');
  Serial.flush();
  delay(5);
  return can.send(canId, data, sizeof(data));
}

static void printEndpointConfig(Stream &out, uint8_t endpointIndex, const EndpointConfig &ep) {
  const uint8_t epNum = endpointIndex + 1;
  const char *units = (ep.pulsesPerRevolution > 0) ? " (deg/deg/s/deg/s²)" : " (device units)";

  out.printf("EP%u: type=%s addr=0x%08lX %s%s\n",
             (unsigned)epNum,
             endpointTypeName(ep.type),
             (unsigned long)ep.address,
             ep.enabled ? "ENABLED" : "DISABLED",
             units);

  out.printf("  Position: [%ld..%ld]", (long)ep.positionMin, (long)ep.positionMax);
  out.printf("  Velocity: [%lu..%lu]", (unsigned long)ep.velocityMin, (unsigned long)ep.velocityMax);
  out.printf("  Accel: [%lu..%lu]\n", (unsigned long)ep.accelMin, (unsigned long)ep.accelMax);

  if (ep.type == EndpointType::RoboClaw) {
    out.printf("  Serial Port: %u  Motor: %u", (unsigned)ep.serialPort, (unsigned)ep.motor);
  } else if (usesCanBus(ep.type)) {
    out.printf("  Interface: CAN");
  } else {
    out.printf("  Interface: Serial %u", (unsigned)ep.serialPort);
  }

  out.printf("  PPR: %lu", (unsigned long)ep.pulsesPerRevolution);
  out.printf("  Home Offset: %ld", (long)ep.homeOffset);
  out.printf("  Home Dir: %s", ep.homeDirection ? "POS" : "NEG");
  out.printf("  Limit: %s\n", ep.hasLimitSwitch ? "YES" : "NO");
}

static uint8_t wrapIndexUp(uint8_t index, uint8_t count) {
  if (count == 0) {
    return 0;
  }
  return (index == 0) ? static_cast<uint8_t>(count - 1) : static_cast<uint8_t>(index - 1);
}

static uint8_t wrapIndexDown(uint8_t index, uint8_t count) {
  if (count == 0) {
    return 0;
  }
  return static_cast<uint8_t>((index + 1) % count);
}

static void rebootNow() {
  Serial.flush();
  delay(50);
#if defined(NVIC_SystemReset)
  NVIC_SystemReset();
#else
  SCB_AIRCR = 0x05FA0004;
  while (true) { }
#endif
}

static void handleConsoleCommand(const CommandMsg &msg) {
  g_app.handleConsoleCommand(msg);
}

/**
 * Description: Initialize application subsystems.
 * Inputs: None.
 * Outputs: Configures input, encoder, UI, show engine, and RS422 ports.
 */
void App::begin() {
  _console.setDispatchCommand(::handleConsoleCommand);
  _console.begin();

  const bool buttonsOk = _buttons.begin();
  const bool ledsOk = _leds.begin();
  _analogs.begin();
  LOGI("Buttons begin: %s", buttonsOk ? "OK" : "FAIL");
  LOGI("Leds begin: %s", ledsOk ? "OK" : "FAIL");

  _enc.begin(PIN_ENC_A, PIN_ENC_B);

  _ui.begin();
  _show.begin();

  // Choose a starting baud for RoboClaw comms; we can change later.
  _rs422.begin(115200);
  _roboclaw.begin(_rs422, 10000, 115200);
  _can.begin(kCanBitrate);

  _configLoaded = _configStore.load(_config);
  if (!_configLoaded) {
    _configStore.setDefaults(_config);
    _configStore.save(_config);
    _configLoaded = true;
  }

  _sdReady = _sd.begin();
  _configFromEndpoints = false;
  if (_sdReady) {
    if (_sd.loadEndpointConfig(_config, Serial)) {
      _configFromEndpoints = true;
      _configStore.save(_config);
    }
  }
  _sequenceLoaded = _sdReady && _sequence.loadFromAnimation(_sd, SdCardManager::kAnimationFilePath, Serial);

  _model.playing = false;
  _model.selectedMotor = 0;
  _screen = UiScreen::Boot;
  _bootStartMs = millis();
  _screenBeforeMenu = UiScreen::Manual;
  _menuIndex = 0;
  _settingsIndex = 0;
  _diagnosticsIndex = 0;
  _endpointConfigIndex = 0;
  _endpointConfigField = 0;
  _endpointConfigEditing = false;
  const char *cfgTag = _configFromEndpoints ? "EP" : (_configLoaded ? "EEP" : "DEF");
  setStatusLine("CFG:%s SD:%s", cfgTag, _sdReady ? "OK" : "ERR");
}

/**
 * Description: Main application loop to update state and render UI.
 * Inputs: None.
 * Outputs: Updates model values and drives outputs each tick.
 */
void App::loop() {
  // Feed watchdog at start of loop
  wdt.feed();

  // Process console commands via USB serial of Teensy 4.1
  _console.poll();

  // Process CAN bus received frames
  _can.processRxFrames();

  // Process button inputs from user interface.
  ButtonState buttonState = _buttons.poll();
  AnalogState analogState = _analogs.read();

  // Update the jog wheel encoder counts.
  const int32_t encoderDelta = _enc.consumeDelta();
  const uint32_t nowMs = _show.currentTimeMs();
  if (_screen == UiScreen::Manual || _screen == UiScreen::Auto) {
    _lastRunScreen = _screen;
  }
  if (_screen == UiScreen::Boot && (millis() - _bootStartMs) > 500) {
    _screen = UiScreen::Manual;
  }

  if (buttonState.justPressed(Button::BUTTON_RIGHT)) {
    if (_screen != UiScreen::Menu && _screen != UiScreen::Settings && _screen != UiScreen::Diagnostics) {
      if (_screen == UiScreen::Edit) {
        saveAnimationEdits();
      }
      _screenBeforeMenu = _lastRunScreen;
      _screen = UiScreen::Menu;
    }
  }

  if (_screen == UiScreen::Menu) {
    if (buttonState.justPressed(Button::BUTTON_UP)) {
      _menuIndex = wrapIndexUp(_menuIndex, MENU_ITEM_COUNT);
    } else if (buttonState.justPressed(Button::BUTTON_DOWN)) {
      _menuIndex = wrapIndexDown(_menuIndex, MENU_ITEM_COUNT);
    } else if (buttonState.justPressed(Button::BUTTON_OK)) {
      const MenuItem &item = MENU_ITEMS[_menuIndex];
      if (item.callback) {
        item.callback(*this);
      }
      if (item.opensScreen) {
        _screen = item.targetScreen;
      }
    } else if (buttonState.justPressed(Button::BUTTON_LEFT)) {
      _screen = _screenBeforeMenu;
    }
  } else if (_screen == UiScreen::EndpointConfig) {
    if (buttonState.justPressed(Button::BUTTON_UP)) {
      _endpointConfigIndex = wrapIndexUp(_endpointConfigIndex, MAX_ENDPOINTS);
    } else if (buttonState.justPressed(Button::BUTTON_DOWN)) {
      _endpointConfigIndex = wrapIndexDown(_endpointConfigIndex, MAX_ENDPOINTS);
    } else if (buttonState.justPressed(Button::BUTTON_OK)) {
      _endpointConfigField = 0;
      _endpointConfigEditing = false;
      _screen = UiScreen::EndpointConfigEdit;
    } else if (buttonState.justPressed(Button::BUTTON_LEFT)) {
      _screen = UiScreen::Menu;
    }
  } else if (_screen == UiScreen::EndpointConfigEdit) {
    if (buttonState.justPressed(Button::BUTTON_LEFT)) {
      _endpointConfigEditing = false;
      _screen = UiScreen::EndpointConfig;
    } else if (buttonState.justPressed(Button::BUTTON_OK)) {
      _endpointConfigEditing = !_endpointConfigEditing;
    } else if (buttonState.justPressed(Button::BUTTON_UP)) {
      if (_endpointConfigEditing) {
        adjustEndpointField(1);
      } else {
        _endpointConfigField = wrapIndexUp(_endpointConfigField, ENDPOINT_FIELD_COUNT);
      }
    } else if (buttonState.justPressed(Button::BUTTON_DOWN)) {
      if (_endpointConfigEditing) {
        adjustEndpointField(-1);
      } else {
        _endpointConfigField = wrapIndexDown(_endpointConfigField, ENDPOINT_FIELD_COUNT);
      }
    }
  } else if (_screen == UiScreen::Edit) {
    if (buttonState.justPressed(Button::BUTTON_LEFT)) {
      saveAnimationEdits();
      _screen = UiScreen::Menu;
    } else if (buttonState.justPressed(Button::BUTTON_OK)) {
      _editField = static_cast<uint8_t>((_editField + 1) % kEditFieldCount);
      _editPosTickAccum = 0;
    } else if (buttonState.justPressed(Button::BUTTON_UP) || buttonState.justPressed(Button::BUTTON_DOWN)) {
      const int direction = buttonState.justPressed(Button::BUTTON_DOWN) ? 1 : -1;
      const size_t total = _sequence.eventCount();
      if (total > 0) {
        size_t indices[SequencePlayer::kMaxEvents] = {};
        size_t count = 0;
        const uint8_t endpointId = static_cast<uint8_t>(_model.selectedMotor + 1);
        for (size_t i = 0; i < total; i++) {
          SequenceEvent ev;
          if (!_sequence.getEvent(i, ev)) {
            break;
          }
          if (ev.endpointId == endpointId) {
            indices[count++] = i;
          }
        }
        if (count > 0) {
          size_t ordinal = 0;
          bool found = false;
          for (size_t i = 0; i < count; i++) {
            if (indices[i] == _editEventIndex) {
              ordinal = i;
              found = true;
              break;
            }
          }
          if (!found) {
            ordinal = 0;
            _editEventIndex = indices[0];
          }
          if (direction > 0) {
            ordinal = (ordinal + 1) % count;
          } else {
            ordinal = (ordinal + count - 1) % count;
          }
          _editEventIndex = indices[ordinal];
        }
      }
    } else if (buttonState.justPressed(Button::BUTTON_YELLOW)) {
      SequenceEvent ev;
      uint16_t ordinal = 0;
      uint16_t count = 0;
      const bool hasEvent = getEditEvent(ev, ordinal, count);
      if (!hasEvent) {
        ev.timeMs = 0;
        ev.position = 0;
        ev.velocity = 0;
        ev.accel = 0;
        ev.mode = SequenceEvent::Mode::Position;
      } else {
        const uint32_t nextTime = ev.timeMs + kEditNewEventOffsetMs;
        ev.timeMs = (nextTime > kEditTimeMaxMs) ? kEditTimeMaxMs : nextTime;
      }
      ev.endpointId = static_cast<uint8_t>(_model.selectedMotor + 1);
      size_t newIndex = _editEventIndex;
      if (_sequence.insertEvent(ev, &newIndex)) {
        _editEventIndex = newIndex;
        setStatusLine("EDIT: ADD");
      } else {
        setStatusLine("EDIT: FULL");
      }
    } else if (buttonState.justPressed(Button::BUTTON_RED)) {
      SequenceEvent ev;
      uint16_t ordinal = 0;
      uint16_t count = 0;
      if (getEditEvent(ev, ordinal, count)) {
        size_t matchIndex = _sequence.eventCount();
        const size_t total = _sequence.eventCount();
        for (size_t i = 0; i < total; i++) {
          SequenceEvent candidate;
          if (!_sequence.getEvent(i, candidate)) {
            break;
          }
          if (candidate.timeMs == ev.timeMs &&
              candidate.endpointId == ev.endpointId &&
              candidate.position == ev.position &&
              candidate.velocity == ev.velocity &&
              candidate.accel == ev.accel &&
              candidate.mode == ev.mode) {
            matchIndex = i;
            break;
          }
        }
        if (matchIndex >= total) {
          matchIndex = _editEventIndex;
        }
        if (_sequence.deleteEvent(matchIndex)) {
          setStatusLine("EDIT: DEL");
          const uint8_t endpointId = static_cast<uint8_t>(_model.selectedMotor + 1);
          if (!selectNeighborEditEvent(endpointId, matchIndex)) {
            _editEventIndex = 0;
          }
        } else {
          setStatusLine("EDIT: DEL ERR");
        }
      }
    } else if (buttonState.justPressed(Button::BUTTON_GREEN)) {
      _model.selectedMotor = wrapIndexDown(_model.selectedMotor, MAX_ENDPOINTS);
      selectFirstEditEvent(static_cast<uint8_t>(_model.selectedMotor + 1));
    }
  } else if (_screen == UiScreen::Endpoints) {
    if (buttonState.justPressed(Button::BUTTON_UP)) {
      _model.selectedMotor = wrapIndexUp(_model.selectedMotor, MAX_ENDPOINTS);
    } else if (buttonState.justPressed(Button::BUTTON_DOWN)) {
      _model.selectedMotor = wrapIndexDown(_model.selectedMotor, MAX_ENDPOINTS);
    } else if (buttonState.justPressed(Button::BUTTON_LEFT)) {
      _screen = UiScreen::Menu;
    }
  } else if (_screen == UiScreen::Settings) {
    if (buttonState.justPressed(Button::BUTTON_UP)) {
      _settingsIndex = wrapIndexUp(_settingsIndex, SETTINGS_ITEM_COUNT);
    } else if (buttonState.justPressed(Button::BUTTON_DOWN)) {
      _settingsIndex = wrapIndexDown(_settingsIndex, SETTINGS_ITEM_COUNT);
    } else if (buttonState.justPressed(Button::BUTTON_OK)) {
      const MenuItem &item = SETTINGS_ITEMS[_settingsIndex];
      if (item.callback) {
        item.callback(*this);
      }
    } else if (buttonState.justPressed(Button::BUTTON_LEFT)) {
      _screen = UiScreen::Menu;
    }
  } else if (_screen == UiScreen::Diagnostics) {
    if (buttonState.justPressed(Button::BUTTON_UP)) {
      _diagnosticsIndex = wrapIndexUp(_diagnosticsIndex, DIAGNOSTICS_ITEM_COUNT);
    } else if (buttonState.justPressed(Button::BUTTON_DOWN)) {
      _diagnosticsIndex = wrapIndexDown(_diagnosticsIndex, DIAGNOSTICS_ITEM_COUNT);
    } else if (buttonState.justPressed(Button::BUTTON_OK)) {
      const MenuItem &item = DIAGNOSTICS_ITEMS[_diagnosticsIndex];
      if (item.callback) {
        item.callback(*this);
      }
      if (item.opensScreen) {
        _screen = item.targetScreen;
      }
    } else if (buttonState.justPressed(Button::BUTTON_LEFT)) {
      _screen = UiScreen::Menu;
    }
  } else if (_screen == UiScreen::RoboClawStatus) {
    if (buttonState.justPressed(Button::BUTTON_LEFT)) {
      _screen = UiScreen::Diagnostics;
    }
  } else {
    if (buttonState.justPressed(Button::BUTTON_YELLOW)) {
      if (_screen == UiScreen::Manual) {
        _screen = UiScreen::Auto;
        _show.begin();
        _show.setPlaying(false);
        _model.playing = false;
        _sequence.reset();
      } else if (_screen == UiScreen::Auto) {
        _screen = UiScreen::Manual;
        _show.setPlaying(false);
        _model.playing = false;
        stopRoboClaws();
      }
    }
    if (buttonState.justPressed(Button::BUTTON_RED)) {
      _model.playing = !_model.playing;
      _show.setPlaying(_model.playing);
      _leds.setMode(LED::LED_RED_BUTTON, _model.playing ? LedMode::On : LedMode::Off);
      if (!_model.playing) {
        stopRoboClaws();
      }
    }
    if (buttonState.justPressed(Button::BUTTON_UP) && _model.selectedMotor > 0) {
      _model.selectedMotor--;
    }
    if (buttonState.justPressed(Button::BUTTON_DOWN) && _model.selectedMotor < (MAX_ENDPOINTS - 1)) {
      _model.selectedMotor++;
    }
    if (buttonState.justPressed(Button::BUTTON_GREEN)) {
      const EndpointConfig &endpoint = _config.endpoints[_model.selectedMotor];
      if (!endpoint.enabled) {
        setStatusLine("MOVE EP DIS");
      } else if (endpoint.type == EndpointType::RoboClaw) {
        const EndpointConfig *ep = nullptr;
        uint8_t portIndex = 0;
        if (resolveEndpoint(_config, _model.selectedMotor, ep, portIndex)) {
          uint32_t maxVel = ep->velocityMax;
          uint32_t minVel = ep->velocityMin;
          if (maxVel == 0 && minVel == 0) {
            maxVel = kMaxVelocityCountsPerSec;
          }
          if (maxVel < minVel) {
            maxVel = minVel;
          }
          uint32_t maxAcc = ep->accelMax;
          uint32_t minAcc = ep->accelMin;
          if (maxAcc == 0 && minAcc == 0) {
            maxAcc = kMaxAccelCountsPerSec2;
          }
          if (maxAcc < minAcc) {
            maxAcc = minAcc;
          }
          const uint32_t velocity = minVel + static_cast<uint32_t>(_model.speedNorm * (maxVel - minVel));
          const uint32_t accel = minAcc + static_cast<uint32_t>(_model.accelNorm * (maxAcc - minAcc));
          int32_t pos = _model.jogPos;
          if (ep->positionMax > ep->positionMin) {
            pos = clampI32(pos, ep->positionMin, ep->positionMax);
          }

          // Convert engineering units to device units if enabled
          int32_t devicePos = UnitConverter::degreesToPulses(static_cast<float>(pos), *ep);
          uint32_t deviceVel = UnitConverter::degPerSecToDeviceVelocity(static_cast<float>(velocity), *ep);
          uint32_t deviceAccel = UnitConverter::degPerSec2ToDeviceAccel(static_cast<float>(accel), *ep);

          uint32_t position = static_cast<uint32_t>(devicePos);
          const bool ok = _roboclaw.commandPosition(portIndex, static_cast<uint8_t>(ep->address), ep->motor, position, deviceVel, deviceAccel);
          setStatusLine("MOVE %s", ok ? "OK" : "FAIL");
        } else {
          setStatusLine("MOVE EP ERR");
        }
      } else if (usesCanBus(endpoint.type)) {
        uint32_t maxVel = endpoint.velocityMax;
        uint32_t minVel = endpoint.velocityMin;
        if (maxVel == 0 && minVel == 0) {
          maxVel = kMaxVelocityCountsPerSec;
        }
        if (maxVel < minVel) {
          maxVel = minVel;
        }
        uint32_t maxAcc = endpoint.accelMax;
        uint32_t minAcc = endpoint.accelMin;
        if (maxAcc == 0 && minAcc == 0) {
          maxAcc = kMaxAccelCountsPerSec2;
        }
        if (maxAcc < minAcc) {
          maxAcc = minAcc;
        }
        const uint32_t velocity = minVel + static_cast<uint32_t>(_model.speedNorm * (maxVel - minVel));
        const uint32_t accel = minAcc + static_cast<uint32_t>(_model.accelNorm * (maxAcc - minAcc));
        const bool ok = (endpoint.type == EndpointType::MksServo) ?
                        sendMksServoPosition(_can, endpoint, _model.jogPos, velocity, accel) :
                        false;
        setStatusLine("MOVE %s", ok ? "OK" : "FAIL");
      } else {
        setStatusLine("MOVE EP ERR");
      }
    }
  }

  if (_screen == UiScreen::EndpointConfigEdit && _endpointConfigEditing && encoderDelta != 0) {
    adjustEndpointField(encoderDelta);
  } else if (_screen == UiScreen::Edit && encoderDelta != 0) {
    SequenceEvent ev;
    uint16_t ordinal = 0;
    uint16_t count = 0;
    if (getEditEvent(ev, ordinal, count)) {
      const EndpointConfig &ep = _config.endpoints[_model.selectedMotor];
      ev.endpointId = static_cast<uint8_t>(_model.selectedMotor + 1);
      switch (static_cast<EditField>(_editField)) {
        case EditField::Time: {
          const int64_t value = static_cast<int64_t>(ev.timeMs) +
                                (static_cast<int64_t>(encoderDelta) * kEditTimeStepMs);
          if (value <= 0) {
            ev.timeMs = 0;
          } else if (value >= static_cast<int64_t>(kEditTimeMaxMs)) {
            ev.timeMs = kEditTimeMaxMs;
          } else {
            ev.timeMs = static_cast<uint32_t>(value);
          }
          break;
        }
        case EditField::Position: {
          _editPosTickAccum += encoderDelta;
          const int32_t stepTicks = kEditPositionTicksPerStep;
          const int32_t steps = (stepTicks != 0) ? (_editPosTickAccum / stepTicks) : 0;
          if (steps == 0) {
            break;
          }
          _editPosTickAccum -= steps * stepTicks;
          const int64_t value = static_cast<int64_t>(ev.position) +
                                (static_cast<int64_t>(steps) * kEditPositionStep);
          const int32_t pos = clampI32Range(static_cast<int32_t>(value), ep.positionMin, ep.positionMax);
          ev.position = pos;
          break;
        }
        case EditField::Velocity: {
          const int64_t value = static_cast<int64_t>(ev.velocity) +
                                (static_cast<int64_t>(encoderDelta) * kEditVelocityStep);
          const uint32_t vel = (value <= 0) ? 0u : static_cast<uint32_t>(value);
          ev.velocity = clampU32Range(vel, ep.velocityMin, ep.velocityMax);
          break;
        }
        case EditField::Accel: {
          const int64_t value = static_cast<int64_t>(ev.accel) +
                                (static_cast<int64_t>(encoderDelta) * kEditAccelStep);
          const uint32_t acc = (value <= 0) ? 0u : static_cast<uint32_t>(value);
          ev.accel = clampU32Range(acc, ep.accelMin, ep.accelMax);
          break;
        }
        default:
          break;
      }
      _sequence.setEvent(_editEventIndex, ev, nullptr, true);
    }
  } else if (_screen != UiScreen::EndpointConfig && _screen != UiScreen::EndpointConfigEdit) {
    // Update jog position - convert encoder ticks to appropriate units
    if (_model.selectedMotor < MAX_ENDPOINTS) {
      const EndpointConfig &ep = _config.endpoints[_model.selectedMotor];
      if (UnitConverter::usesEngineeringUnits(ep)) {
        // Engineering units mode: 100 encoder ticks = 360 degrees (1 full rotation)
        const float degreesPerTick = 360.0f / 100.0f;
        const float degrees = encoderDelta * degreesPerTick;
        _model.jogPos = static_cast<int32_t>(static_cast<float>(_model.jogPos) + degrees);
      } else {
        // Device units mode: direct tick-to-pulse mapping
        _model.jogPos += encoderDelta;
      }
    } else {
      _model.jogPos += encoderDelta;
    }
  }

  if (_screen == UiScreen::Auto && _model.playing && _sequence.loaded()) {
    _sequence.update(nowMs, _roboclaw, _can, _config);
  }

  _can.events();
  _can.dumpRxLog(Serial);
  _can.logErrorCounters(Serial, millis());

  pollRoboClaws();
  pollCanEndpoints();

  const EndpointConfig *selectedEp = nullptr;
  uint8_t selectedPortIndex = 0;
  if (resolveEndpoint(_config, _model.selectedMotor, selectedEp, selectedPortIndex)) {
    const uint8_t endpointIndex = _model.selectedMotor;
    if (_rcStatusByEndpointValid[endpointIndex]) {
      _rcStatus = _rcStatusByEndpoint[endpointIndex];
      _rcStatusValid = true;
    } else {
      _rcStatus = {};
      _rcStatusValid = false;
    }
  } else {
    _rcStatus = {};
    _rcStatusValid = false;
  }

  uint32_t displayTimeMs = nowMs;
  const uint32_t loopMs = _sequence.loopMs();
  if (_sequence.loaded() && loopMs > 0) {
    displayTimeMs = nowMs % (loopMs + 1);
  }
  _model.showTimeMs = displayTimeMs;
  _model.speedNorm  = analogState.potSpeedNorm;
  _model.accelNorm  = analogState.potAccelNorm;
  _model.screen = _screen;
  _model.menuIndex = _menuIndex;
  _model.settingsIndex = _settingsIndex;
  _model.diagnosticsIndex = _diagnosticsIndex;
  _model.endpointConfigIndex = _endpointConfigIndex;
  _model.endpointConfigField = _endpointConfigField;
  _model.endpointConfigEditing = _endpointConfigEditing;
  _model.sdReady = _sdReady;
  strncpy(_model.statusLine, _statusLine, sizeof(_model.statusLine));
  _model.statusLine[sizeof(_model.statusLine) - 1] = '\0';
  _model.rcStatusValid = _rcStatusValid;
  _model.rcEncValid = _rcStatus.encValid && _rcStatusValid;
  _model.rcSpeedValid = _rcStatus.speedValid && _rcStatusValid;
  _model.rcErrorValid = _rcStatus.errorValid && _rcStatusValid;
  _model.rcEnc1 = _rcStatus.enc1;
  _model.rcEnc2 = _rcStatus.enc2;
  _model.rcSpeed1 = _rcStatus.speed1;
  _model.rcSpeed2 = _rcStatus.speed2;
  _model.rcError = _rcStatus.error;
  if (_model.rcEncValid && selectedEp) {
    _model.rcSelectedEnc = (selectedEp->motor == 1) ? _rcStatus.enc1 : _rcStatus.enc2;
  } else {
    _model.rcSelectedEnc = 0;
  }
  if (_model.rcSpeedValid && selectedEp) {
    _model.rcSelectedSpeed = (selectedEp->motor == 1) ? _rcStatus.speed1 : _rcStatus.speed2;
  } else {
    _model.rcSelectedSpeed = 0;
  }
  _sequenceLoaded = _sequence.loaded();
  _model.sequenceLoaded = _sequenceLoaded;
  _model.sequenceCount = static_cast<uint16_t>(_sequence.eventCount());
  _model.sequenceLoopMs = _sequence.loopMs();
  _model.editField = _editField;
  {
    SequenceEvent editEvent;
    uint16_t ordinal = 0;
    uint16_t count = 0;
    const bool hasEvent = getEditEvent(editEvent, ordinal, count);
    _model.editHasEvent = hasEvent;
    _model.editEventOrdinal = ordinal;
    _model.editEventCount = count;
    if (hasEvent) {
      _model.editTimeMs = editEvent.timeMs;
      _model.editPosition = editEvent.position;
      _model.editVelocity = editEvent.velocity;
      _model.editAccel = editEvent.accel;
    } else {
      _model.editTimeMs = 0;
      _model.editPosition = 0;
      _model.editVelocity = 0;
      _model.editAccel = 0;
    }
  }
  if (_endpointConfigIndex < MAX_ENDPOINTS) {
    _model.endpointConfigSelected = _config.endpoints[_endpointConfigIndex];
  }

  for (uint8_t i = 0; i < MAX_RC_PORTS; i++) {
    _model.rcPortEnabled[i] = false;
    _model.rcPortAddress[i] = 0;
    _model.rcPortStatusValid[i] = false;
    _model.rcPortEncValid[i] = false;
    _model.rcPortSpeedValid[i] = false;
    _model.rcPortErrorValid[i] = false;
    _model.rcPortEnc1[i] = 0;
    _model.rcPortEnc2[i] = 0;
    _model.rcPortSpeed1[i] = 0;
    _model.rcPortSpeed2[i] = 0;
    _model.rcPortError[i] = 0;
  }

  for (uint8_t i = 0; i < MAX_ENDPOINTS; i++) {
    const EndpointConfig &ep = _config.endpoints[i];
    if (!ep.enabled) {
      continue;
    }
    if (ep.type != EndpointType::RoboClaw || ep.address > 0xFF) {
      continue;
    }
    if (ep.serialPort < 1 || ep.serialPort > MAX_RC_PORTS) {
      continue;
    }
    const uint8_t portIndex = static_cast<uint8_t>(ep.serialPort - 1);
    const bool statusValid = _rcStatusByEndpointValid[i];
    const RoboClawStatus &status = _rcStatusByEndpoint[i];

    if (!_model.rcPortEnabled[portIndex] || (!_model.rcPortStatusValid[portIndex] && statusValid)) {
      _model.rcPortEnabled[portIndex] = true;
      _model.rcPortAddress[portIndex] = static_cast<uint8_t>(ep.address);
      _model.rcPortStatusValid[portIndex] = statusValid;
      _model.rcPortEncValid[portIndex] = statusValid && status.encValid;
      _model.rcPortSpeedValid[portIndex] = statusValid && status.speedValid;
      _model.rcPortErrorValid[portIndex] = statusValid && status.errorValid;
      _model.rcPortEnc1[portIndex] = status.enc1;
      _model.rcPortEnc2[portIndex] = status.enc2;
      _model.rcPortSpeed1[portIndex] = status.speed1;
      _model.rcPortSpeed2[portIndex] = status.speed2;
      _model.rcPortError[portIndex] = status.error;
    }
  }

  for (uint8_t i = 0; i < MAX_ENDPOINTS; i++) {
    const EndpointConfig &ep = _config.endpoints[i];
    _model.endpointEnabled[i] = (ep.enabled != 0);
    _model.endpointConfigType[i] = ep.type;
    _model.endpointConfigPort[i] = ep.serialPort;
    _model.endpointConfigMotor[i] = ep.motor;
    _model.endpointConfigAddress[i] = ep.address;
    if (!_model.endpointEnabled[i]) {
      _model.endpointStatusValid[i] = false;
      _model.endpointEncValid[i] = false;
      _model.endpointSpeedValid[i] = false;
      _model.endpointPos[i] = 0;
      _model.endpointSpeed[i] = 0;
      continue;
    }
    if (ep.type != EndpointType::RoboClaw || ep.address > 0xFF) {
      _model.endpointStatusValid[i] = false;
      _model.endpointEncValid[i] = false;
      _model.endpointSpeedValid[i] = false;
      _model.endpointPos[i] = 0;
      _model.endpointSpeed[i] = 0;
      continue;
    }
    const RoboClawStatus &status = _rcStatusByEndpoint[i];
    const bool statusValid = _rcStatusByEndpointValid[i];
    _model.endpointStatusValid[i] = statusValid;
    _model.endpointEncValid[i] = statusValid && status.encValid;
    _model.endpointSpeedValid[i] = statusValid && status.speedValid;
    if (ep.motor == 1) {
      _model.endpointPos[i] = status.enc1;
      _model.endpointSpeed[i] = status.speed1;
    } else {
      _model.endpointPos[i] = status.enc2;
      _model.endpointSpeed[i] = status.speed2;
    }
  }

  _leds.update();

  // Update the user interface outputs with latest status.
  _ui.render(_model);
}

void App::pollRoboClaws() {
  const uint32_t now = millis();
  if ((now - _lastRcPollMs) < kRcPollPeriodMs) {
    return;
  }
  _lastRcPollMs = now;

  for (uint8_t i = 0; i < MAX_ENDPOINTS; ++i) {
    const uint8_t endpointIndex = static_cast<uint8_t>((_rcPollIndex + i) % MAX_ENDPOINTS);
    const EndpointConfig *ep = nullptr;
    uint8_t portIndex = 0;
    if (!resolveEndpoint(_config, endpointIndex, ep, portIndex)) {
      continue;
    }
    RoboClawStatus status;
    const bool ok = _roboclaw.readStatus(portIndex, static_cast<uint8_t>(ep->address), status);
    if (ok) {
      _rcStatusByEndpoint[endpointIndex] = status;
    }
    _rcStatusByEndpointValid[endpointIndex] = ok;
    _rcPollIndex = static_cast<uint8_t>((endpointIndex + 1) % MAX_ENDPOINTS);
    return;
  }
}

void App::pollCanEndpoints() {
  const uint32_t now = millis();
  if ((now - _lastCanPollMs) < kRcPollPeriodMs) {
    return;
  }
  _lastCanPollMs = now;

  // Poll one CAN endpoint per cycle (round-robin)
  for (uint8_t i = 0; i < MAX_ENDPOINTS; ++i) {
    const uint8_t epIndex = static_cast<uint8_t>((_canPollIndex + i) % MAX_ENDPOINTS);
    const EndpointConfig &ep = _config.endpoints[epIndex];

    // Only poll enabled MKS Servo endpoints
    if (!ep.enabled || ep.type != EndpointType::MksServo) {
      continue;
    }
    if (ep.address > 0x7FF) {
      continue;
    }

    // Request status from this servo
    uint16_t canId = static_cast<uint16_t>(ep.address);
    _can.requestMksServoStatus(canId);

    // Try to get cached status
    MksServoStatus status;
    if (_can.getMksServoStatus(canId, status)) {
      _canStatusByEndpoint[epIndex] = status;
      _canStatusByEndpointValid[epIndex] = true;
    }

    // Advance to next endpoint for next poll
    _canPollIndex = static_cast<uint8_t>((epIndex + 1) % MAX_ENDPOINTS);
    return;  // Only poll one endpoint per cycle
  }
}

void App::stopRoboClaws() {
  for (uint8_t i = 0; i < MAX_ENDPOINTS; ++i) {
    const EndpointConfig *ep = nullptr;
    uint8_t portIndex = 0;
    if (!resolveEndpoint(_config, i, ep, portIndex)) {
      continue;
    }
    uint32_t maxVel = ep->velocityMax;
    uint32_t minVel = ep->velocityMin;
    if (maxVel == 0 && minVel == 0) {
      maxVel = kMaxVelocityCountsPerSec;
    }
    if (maxVel < minVel) {
      maxVel = minVel;
    }
    uint32_t maxAcc = ep->accelMax;
    uint32_t minAcc = ep->accelMin;
    if (maxAcc == 0 && minAcc == 0) {
      maxAcc = kMaxAccelCountsPerSec2;
    }
    if (maxAcc < minAcc) {
      maxAcc = minAcc;
    }

    if (_rcStatusByEndpointValid[i] && _rcStatusByEndpoint[i].encValid) {
      int32_t pos = (ep->motor == 1) ? _rcStatusByEndpoint[i].enc1 : _rcStatusByEndpoint[i].enc2;
      if (ep->positionMax > ep->positionMin) {
        pos = clampI32(pos, ep->positionMin, ep->positionMax);
      }
      _roboclaw.commandPosition(portIndex, static_cast<uint8_t>(ep->address), ep->motor,
                                static_cast<uint32_t>(pos), maxVel, maxAcc);
    } else {
      _roboclaw.commandVelocity(portIndex, static_cast<uint8_t>(ep->address), ep->motor, 0, maxAcc);
    }
  }
}

void App::saveAnimationEdits() {
  if (!_sdReady) {
    setStatusLine("EDIT: SD ERR");
    return;
  }
  _sequence.sortForPlayback();
  if (_sequence.saveToAnimation(_sd, SdCardManager::kAnimationFilePath, Serial)) {
    setStatusLine("EDIT: SAVED");
  } else {
    setStatusLine("EDIT: SAVE ERR");
  }
}

bool App::getEditEvent(SequenceEvent &event, uint16_t &ordinal, uint16_t &count) {
  ordinal = 0;
  count = 0;
  const size_t total = _sequence.eventCount();
  if (total == 0) {
    return false;
  }
  const uint8_t endpointId = static_cast<uint8_t>(_model.selectedMotor + 1);
  bool found = false;
  for (size_t i = 0; i < total; i++) {
    SequenceEvent ev;
    if (!_sequence.getEvent(i, ev)) {
      break;
    }
    if (ev.endpointId != endpointId) {
      continue;
    }
    count++;
    if (i == _editEventIndex) {
      event = ev;
      ordinal = count;
      found = true;
    }
  }

  if (!found && count > 0) {
    for (size_t i = 0; i < total; i++) {
      SequenceEvent ev;
      if (!_sequence.getEvent(i, ev)) {
        break;
      }
      if (ev.endpointId == endpointId) {
        _editEventIndex = i;
        event = ev;
        ordinal = 1;
        found = true;
        break;
      }
    }
  }

  return found;
}

bool App::selectFirstEditEvent(uint8_t endpointId) {
  const size_t total = _sequence.eventCount();
  for (size_t i = 0; i < total; i++) {
    SequenceEvent ev;
    if (!_sequence.getEvent(i, ev)) {
      break;
    }
    if (ev.endpointId == endpointId) {
      _editEventIndex = i;
      return true;
    }
  }
  return false;
}

bool App::selectNeighborEditEvent(uint8_t endpointId, size_t startIndex) {
  const size_t total = _sequence.eventCount();
  if (total == 0) {
    return false;
  }
  for (size_t i = startIndex; i < total; i++) {
    SequenceEvent ev;
    if (!_sequence.getEvent(i, ev)) {
      break;
    }
    if (ev.endpointId == endpointId) {
      _editEventIndex = i;
      return true;
    }
  }
  if (startIndex > 0) {
    for (size_t i = startIndex; i-- > 0;) {
      SequenceEvent ev;
      if (!_sequence.getEvent(i, ev)) {
        break;
      }
      if (ev.endpointId == endpointId) {
        _editEventIndex = i;
        return true;
      }
    }
  }
  return false;
}

void App::adjustEndpointField(int32_t delta) {
  if (delta == 0) {
    return;
  }
  if (_endpointConfigIndex >= MAX_ENDPOINTS) {
    return;
  }
  EndpointConfig &ep = _config.endpoints[_endpointConfigIndex];
  bool changed = false;
  switch (static_cast<EndpointField>(_endpointConfigField)) {
    case EndpointField::Enabled:
      if (ep.enabled != ((delta > 0) ? 1 : 0)) {
        ep.enabled = (delta > 0) ? 1 : 0;
        changed = true;
      }
      break;
    case EndpointField::Type: {
      const EndpointType prevType = ep.type;
      const uint8_t prevPort = ep.serialPort;
      const uint8_t prevMotor = ep.motor;
      int32_t value = static_cast<int32_t>(ep.type) + delta;
      if (value < 0) {
        value = static_cast<int32_t>(EndpointType::JoeServoCan);
      } else if (value > static_cast<int32_t>(EndpointType::JoeServoCan)) {
        value = 0;
      }
      ep.type = static_cast<EndpointType>(value);
      if (usesCanBus(ep.type)) {
        ep.serialPort = 0;
        ep.motor = 0;
      } else if (ep.serialPort < 1) {
        ep.serialPort = 1;
        if (ep.type == EndpointType::RoboClaw && ep.motor == 0) {
          ep.motor = 1;
        }
      }
      changed = (ep.type != prevType || ep.serialPort != prevPort || ep.motor != prevMotor);
      break;
    }
    case EndpointField::Address: {
      const int64_t value = static_cast<int64_t>(ep.address) + delta;
      const uint32_t next = clampU32(value, kEndpointAddressMin, kEndpointAddressMax);
      if (next != ep.address) {
        ep.address = next;
        changed = true;
      }
      break;
    }
    case EndpointField::SerialPort: {
      const uint8_t prevPort = ep.serialPort;
      const int32_t value = static_cast<int32_t>(ep.serialPort) + delta;
      ep.serialPort = clampU8(value, kEndpointPortMin, kEndpointPortMax);
      if (usesCanBus(ep.type)) {
        ep.serialPort = 0;
      } else if (ep.serialPort < 1) {
        ep.serialPort = 1;
      }
      changed = (ep.serialPort != prevPort);
      break;
    }
    case EndpointField::Motor: {
      const uint8_t prevMotor = ep.motor;
      const int32_t value = static_cast<int32_t>(ep.motor) + delta;
      ep.motor = clampU8(value, kEndpointMotorMin, kEndpointMotorMax);
      if (usesCanBus(ep.type)) {
        ep.motor = 0;
      }
      changed = (ep.motor != prevMotor);
      break;
    }
    case EndpointField::PositionMin: {
      const int64_t value = static_cast<int64_t>(ep.positionMin) + (static_cast<int64_t>(delta) * kEndpointPositionStep);
      const int32_t next = clampI32(value, INT32_MIN, INT32_MAX);
      if (next != ep.positionMin) {
        ep.positionMin = next;
        changed = true;
      }
      break;
    }
    case EndpointField::PositionMax: {
      const int64_t value = static_cast<int64_t>(ep.positionMax) + (static_cast<int64_t>(delta) * kEndpointPositionStep);
      const int32_t next = clampI32(value, INT32_MIN, INT32_MAX);
      if (next != ep.positionMax) {
        ep.positionMax = next;
        changed = true;
      }
      break;
    }
    case EndpointField::VelocityMin: {
      const int64_t value = static_cast<int64_t>(ep.velocityMin) + (static_cast<int64_t>(delta) * kEndpointVelocityStep);
      const uint32_t next = clampU32(value, 0, kEndpointRateMax);
      if (next != ep.velocityMin) {
        ep.velocityMin = next;
        changed = true;
      }
      break;
    }
    case EndpointField::VelocityMax: {
      const int64_t value = static_cast<int64_t>(ep.velocityMax) + (static_cast<int64_t>(delta) * kEndpointVelocityStep);
      const uint32_t next = clampU32(value, 0, kEndpointRateMax);
      if (next != ep.velocityMax) {
        ep.velocityMax = next;
        changed = true;
      }
      break;
    }
    case EndpointField::AccelMin: {
      const int64_t value = static_cast<int64_t>(ep.accelMin) + (static_cast<int64_t>(delta) * kEndpointAccelStep);
      const uint32_t next = clampU32(value, 0, kEndpointRateMax);
      if (next != ep.accelMin) {
        ep.accelMin = next;
        changed = true;
      }
      break;
    }
    case EndpointField::AccelMax: {
      const int64_t value = static_cast<int64_t>(ep.accelMax) + (static_cast<int64_t>(delta) * kEndpointAccelStep);
      const uint32_t next = clampU32(value, 0, kEndpointRateMax);
      if (next != ep.accelMax) {
        ep.accelMax = next;
        changed = true;
      }
      break;
    }
    case EndpointField::PulsesPerRev: {
      const int64_t value = static_cast<int64_t>(ep.pulsesPerRevolution) + (static_cast<int64_t>(delta) * 1000);
      const uint32_t next = clampU32(value, 0, 1000000);  // Max 1M pulses/rev
      if (next != ep.pulsesPerRevolution) {
        ep.pulsesPerRevolution = next;
        changed = true;
      }
      break;
    }
    case EndpointField::HomeOffset: {
      const int64_t value = static_cast<int64_t>(ep.homeOffset) + (static_cast<int64_t>(delta) * 100);
      const int32_t next = clampI32(value, INT32_MIN, INT32_MAX);
      if (next != ep.homeOffset) {
        ep.homeOffset = next;
        changed = true;
      }
      break;
    }
    case EndpointField::HomeDirection: {
      if (ep.homeDirection != ((delta > 0) ? 1 : 0)) {
        ep.homeDirection = (delta > 0) ? 1 : 0;
        changed = true;
      }
      break;
    }
    case EndpointField::HasLimitSwitch: {
      if (ep.hasLimitSwitch != ((delta > 0) ? 1 : 0)) {
        ep.hasLimitSwitch = (delta > 0) ? 1 : 0;
        changed = true;
      }
      break;
    }
    default:
      break;
  }
  if (changed) {
    actionSaveConfig();
  }
}

/**
 * Description: Arduino setup entry point.
 * Inputs: None.
 * Outputs: Initializes logging and application subsystems.
 */
void watchdogCallback() {
  Serial.println("WATCHDOG TRIGGERED - SYSTEM RESET");
}

void setup() {
  logInit(115200);
  LOGI("Boot");

  // Configure watchdog: 5 second timeout
  WDT_timings_t config;
  config.timeout = 5;     // seconds
  config.trigger = 4;     // reset at 4 seconds
  config.callback = watchdogCallback;
  wdt.begin(config);

  Serial.println("Watchdog enabled (5s timeout)");

  g_app.begin();
}

/**
 * Description: Arduino loop entry point.
 * Inputs: None.
 * Outputs: Runs the application loop continuously.
 */
void loop() {
  g_app.loop();
}

void App::handleConsoleCommand(const CommandMsg &msg) {
  if (strcmp(msg.cmd, "help") == 0) {
    Serial.println("Commands:");
    Serial.println("  help");
    Serial.println("  sd dir [path]");
    Serial.println("  sd read <path>");
    Serial.println("  sd test");
    Serial.println("  config save");
    Serial.println("  config reset");
    Serial.println("  config load endpoints");
    Serial.println("  config show");
    Serial.println("  factory reset");
    Serial.println("  ep list");
    Serial.println("  ep show <endpoint>");
    Serial.println("  ep enable <endpoint>");
    Serial.println("  ep disable <endpoint>");
    Serial.println("  ep set <endpoint> <field> <value>");
    Serial.println("    fields: enabled, type, address, serial, motor, pos_min, pos_max,");
    Serial.println("            vmin, vmax, amin, amax, ppr, home_offset, home_dir, limit");
    Serial.println("  ep save");
    Serial.println("  seq load [path]");
    Serial.println("  seq info");
    Serial.println("  rc status <endpoint>");
    Serial.println("  rc pos <endpoint> <pos> <vel> <accel>");
    Serial.println("  rc vel <endpoint> <vel> <accel>");
    Serial.println("  can status");
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
    const char *sub = msg.argv[0];
    if (strcmp(sub, "dir") == 0) {
      const char *path = (msg.argc > 1) ? msg.argv[1] : "/";
      if (!_sd.listDir(path, Serial)) {
        statusMessage("SD: dir failed");
      }
      return;
    }
    if (strcmp(sub, "read") == 0) {
      if (msg.argc < 2) {
        statusMessage("SD: read requires a path");
        return;
      }
      if (!_sd.readFile(msg.argv[1], Serial)) {
        statusMessage("SD: read failed");
      }
      return;
    }
    if (strcmp(sub, "test") == 0) {
      bool ok = _sd.testReadWrite(Serial);
      statusMessage("SD TEST: %s", ok ? "OK" : "FAIL");
      return;
    }
    statusMessage("SD: unknown subcommand");
    return;
  }

  if (strcmp(msg.cmd, "config") == 0) {
    if (msg.argc == 0) {
      statusMessage("config save | config reset | config load endpoints | config show");
      return;
    }
    if (strcmp(msg.argv[0], "save") == 0) {
      _configStore.save(_config);
      bool sdOk = _sdReady && _sd.saveEndpointConfig(_config, Serial);
      statusMessage("CONFIG: saved (%s)", sdOk ? "EEP+EP" : "EEP");
      return;
    }
    if (strcmp(msg.argv[0], "reset") == 0) {
      actionResetConfig();
      Serial.println("CONFIG: reset to defaults");
      return;
    }
    if (strcmp(msg.argv[0], "load") == 0) {
      if (!_sdReady) {
        statusMessage("CONFIG: SD not ready");
        return;
      }
      if (msg.argc < 2) {
        statusMessage("CONFIG: load endpoints");
        return;
      }
      const char *sub = msg.argv[1];
      if (strcmp(sub, "endpoints") == 0) {
        if (_sd.loadEndpointConfig(_config, Serial)) {
          _configStore.save(_config);
          _configFromEndpoints = true;
          statusMessage("CONFIG: loaded from endpoints");
        } else {
          statusMessage("CONFIG: endpoints load failed");
        }
        return;
      }
      statusMessage("CONFIG: unknown load target");
      return;
    }
    if (strcmp(msg.argv[0], "show") == 0) {
      Serial.printf("CONFIG: endpoints=%u\n", (unsigned)MAX_ENDPOINTS);
      for (uint8_t i = 0; i < MAX_ENDPOINTS; i++) {
        Serial.print("  ");
        printEndpointConfig(Serial, i, _config.endpoints[i]);
      }
      return;
    }
    statusMessage("CONFIG: unknown subcommand");
    return;
  }

  if (strcmp(msg.cmd, "ep") == 0 || strcmp(msg.cmd, "endpoint") == 0) {
    if (msg.argc == 0) {
      statusMessage("ep list | ep show <endpoint> | ep enable <endpoint> | ep disable <endpoint> | ep set <endpoint> <field> <value> | ep save");
      return;
    }
    const char *sub = msg.argv[0];
    if (strcmp(sub, "list") == 0) {
      Serial.printf("ENDPOINTS: %u\n", (unsigned)MAX_ENDPOINTS);
      for (uint8_t i = 0; i < MAX_ENDPOINTS; i++) {
        printEndpointConfig(Serial, i, _config.endpoints[i]);
      }
      return;
    }
    if (strcmp(sub, "show") == 0) {
      if (msg.argc < 2) {
        statusMessage("EP: show requires endpoint");
        return;
      }
      uint32_t endpointId = 0;
      if (!parseUint32(msg.argv[1], endpointId) || endpointId == 0 || endpointId > MAX_ENDPOINTS) {
        statusMessage("EP: invalid endpoint");
        return;
      }
      const uint8_t endpointIndex = static_cast<uint8_t>(endpointId - 1);
      printEndpointConfig(Serial, endpointIndex, _config.endpoints[endpointIndex]);
      return;
    }
    if (strcmp(sub, "enable") == 0 || strcmp(sub, "disable") == 0) {
      if (msg.argc < 2) {
        statusMessage("EP: %s requires endpoint", sub);
        return;
      }
      uint32_t endpointId = 0;
      if (!parseUint32(msg.argv[1], endpointId) || endpointId == 0 || endpointId > MAX_ENDPOINTS) {
        statusMessage("EP: invalid endpoint");
        return;
      }
      const uint8_t endpointIndex = static_cast<uint8_t>(endpointId - 1);
      EndpointConfig &ep = _config.endpoints[endpointIndex];
      ep.enabled = (strcmp(sub, "enable") == 0) ? 1 : 0;
      actionSaveConfig();
      statusMessage("EP%u %s", (unsigned)endpointId, ep.enabled ? "enabled" : "disabled");
      return;
    }
    if (strcmp(sub, "set") == 0) {
      if (msg.argc < 4) {
        statusMessage("EP: set <endpoint> <field> <value>");
        return;
      }
      uint32_t endpointId = 0;
      if (!parseUint32(msg.argv[1], endpointId) || endpointId == 0 || endpointId > MAX_ENDPOINTS) {
        statusMessage("EP: invalid endpoint");
        return;
      }
      const uint8_t endpointIndex = static_cast<uint8_t>(endpointId - 1);
      EndpointField field = EndpointField::Enabled;
      if (!parseEndpointFieldName(msg.argv[2], field)) {
        statusMessage("EP: unknown field");
        return;
      }
      EndpointConfig &ep = _config.endpoints[endpointIndex];
      switch (field) {
        case EndpointField::Enabled: {
          uint8_t enabled = 0;
          if (!parseBoolToken(msg.argv[3], enabled)) {
            statusMessage("EP: enabled expects on/off/0/1");
            return;
          }
          ep.enabled = enabled;
          break;
        }
        case EndpointField::Type: {
          EndpointType type = EndpointType::RoboClaw;
          if (!parseEndpointType(msg.argv[3], type)) {
            statusMessage("EP: type expects ROBOCLAW/MKS_SERVO/REV_FRC_CAN/JOE_SERVO_SERIAL/JOE_SERVO_CAN");
            return;
          }
          ep.type = type;
          if (usesCanBus(ep.type)) {
            ep.serialPort = 0;
            ep.motor = 0;
          } else if (ep.serialPort < 1) {
            ep.serialPort = 1;
            if (ep.type == EndpointType::RoboClaw && ep.motor == 0) {
              ep.motor = 1;
            }
          }
          break;
        }
        case EndpointField::SerialPort: {
          uint32_t port = 0;
          if (!parseUint32(msg.argv[3], port)) {
            statusMessage("EP: serial expects number");
            return;
          }
          ep.serialPort = clampU8(static_cast<int32_t>(port), kEndpointPortMin, kEndpointPortMax);
          if (usesCanBus(ep.type)) {
            ep.serialPort = 0;
          } else if (ep.serialPort < 1) {
            ep.serialPort = 1;
          }
          break;
        }
        case EndpointField::Motor: {
          uint32_t motor = 0;
          if (!parseUint32(msg.argv[3], motor)) {
            statusMessage("EP: motor expects number");
            return;
          }
          ep.motor = clampU8(static_cast<int32_t>(motor), kEndpointMotorMin, kEndpointMotorMax);
          if (usesCanBus(ep.type)) {
            ep.motor = 0;
          }
          break;
        }
        case EndpointField::Address: {
          uint32_t address = 0;
          if (!parseUint32(msg.argv[3], address)) {
            statusMessage("EP: address expects number");
            return;
          }
          ep.address = clampU32(static_cast<int64_t>(address), kEndpointAddressMin, kEndpointAddressMax);
          break;
        }
        case EndpointField::PositionMin: {
          int32_t pmin = 0;
          if (!parseInt32(msg.argv[3], pmin)) {
            statusMessage("EP: pos_min expects number");
            return;
          }
          ep.positionMin = pmin;
          break;
        }
        case EndpointField::PositionMax: {
          int32_t pmax = 0;
          if (!parseInt32(msg.argv[3], pmax)) {
            statusMessage("EP: pos_max expects number");
            return;
          }
          ep.positionMax = pmax;
          break;
        }
        case EndpointField::VelocityMin: {
          uint32_t vmin = 0;
          if (!parseUint32(msg.argv[3], vmin)) {
            statusMessage("EP: vmin expects number");
            return;
          }
          ep.velocityMin = clampU32(static_cast<int64_t>(vmin), 0, kEndpointRateMax);
          break;
        }
        case EndpointField::VelocityMax: {
          uint32_t vmax = 0;
          if (!parseUint32(msg.argv[3], vmax)) {
            statusMessage("EP: vmax expects number");
            return;
          }
          ep.velocityMax = clampU32(static_cast<int64_t>(vmax), 0, kEndpointRateMax);
          break;
        }
        case EndpointField::AccelMin: {
          uint32_t amin = 0;
          if (!parseUint32(msg.argv[3], amin)) {
            statusMessage("EP: amin expects number");
            return;
          }
          ep.accelMin = clampU32(static_cast<int64_t>(amin), 0, kEndpointRateMax);
          break;
        }
        case EndpointField::AccelMax: {
          uint32_t amax = 0;
          if (!parseUint32(msg.argv[3], amax)) {
            statusMessage("EP: amax expects number");
            return;
          }
          ep.accelMax = clampU32(static_cast<int64_t>(amax), 0, kEndpointRateMax);
          break;
        }
        case EndpointField::PulsesPerRev: {
          uint32_t ppr = 0;
          if (!parseUint32(msg.argv[3], ppr)) {
            statusMessage("EP: ppr expects number");
            return;
          }
          ep.pulsesPerRevolution = clampU32(static_cast<int64_t>(ppr), 0, 1000000);
          break;
        }
        case EndpointField::HomeOffset: {
          int32_t offset = 0;
          if (!parseInt32(msg.argv[3], offset)) {
            statusMessage("EP: home_offset expects number");
            return;
          }
          ep.homeOffset = offset;
          break;
        }
        case EndpointField::HomeDirection: {
          uint32_t dir = 0;
          if (!parseUint32(msg.argv[3], dir)) {
            statusMessage("EP: home_dir expects 0 or 1");
            return;
          }
          ep.homeDirection = (dir != 0) ? 1 : 0;
          break;
        }
        case EndpointField::HasLimitSwitch: {
          uint8_t hasLimit = 0;
          if (!parseBoolToken(msg.argv[3], hasLimit)) {
            statusMessage("EP: limit expects on/off/0/1");
            return;
          }
          ep.hasLimitSwitch = hasLimit;
          break;
        }
        default:
          statusMessage("EP: unsupported field");
          return;
      }
      statusMessage("EP%u updated", (unsigned)endpointId);
      actionSaveConfig();
      return;
    }
    if (strcmp(sub, "save") == 0) {
      actionSaveConfig();
      Serial.println("EP: save requested");
      return;
    }
    statusMessage("EP: unknown subcommand");
    return;
  }

  if (strcmp(msg.cmd, "factory") == 0) {
    if (msg.argc == 0) {
      statusMessage("factory reset");
      return;
    }
    if (strcmp(msg.argv[0], "reset") == 0) {
      actionFactoryReset();
      Serial.println("FACTORY: reset requested");
      return;
    }
    statusMessage("FACTORY: unknown subcommand");
    return;
  }

  if (strcmp(msg.cmd, "rc") == 0) {
    if (msg.argc == 0) {
      statusMessage("rc status <endpoint> | rc pos <endpoint> <pos> <vel> <accel> | rc vel <endpoint> <vel> <accel>");
      return;
    }
    const char *sub = msg.argv[0];
    uint32_t endpointId = 0;
    if (msg.argc < 2 || !parseUint32(msg.argv[1], endpointId) || endpointId == 0 || endpointId > MAX_ENDPOINTS) {
      statusMessage("RC: invalid endpoint");
      return;
    }
    const uint8_t endpointIndex = static_cast<uint8_t>(endpointId - 1);
    const EndpointConfig *ep = nullptr;
    uint8_t portIndex = 0;
    if (!resolveEndpoint(_config, endpointIndex, ep, portIndex)) {
      statusMessage("RC: endpoint disabled or invalid");
      return;
    }

    if (strcmp(sub, "status") == 0) {
      RoboClawStatus status;
      if (_roboclaw.readStatus(portIndex, static_cast<uint8_t>(ep->address), status)) {
        Serial.printf("RC: ENC1=%lu ENC2=%lu ERR=0x%08lX\n",
                      (unsigned long)status.enc1,
                      (unsigned long)status.enc2,
                      (unsigned long)status.error);
        setStatusLine("RC: status ok");
      } else {
        statusMessage("RC: status read failed");
      }
      return;
    }
    if (strcmp(sub, "pos") == 0) {
      if (msg.argc < 5) {
        statusMessage("RC: pos requires endpoint pos vel accel");
        return;
      }
      int32_t pos = 0;
      uint32_t vel = 0;
      uint32_t accel = 0;
      if (!parseInt32(msg.argv[2], pos) || !parseUint32(msg.argv[3], vel) || !parseUint32(msg.argv[4], accel)) {
        statusMessage("RC: pos parse error");
        return;
      }
      const bool ok = _roboclaw.commandPosition(portIndex, static_cast<uint8_t>(ep->address), ep->motor,
                                                static_cast<uint32_t>(pos), vel, accel);
      statusMessage("RC: pos %s", ok ? "OK" : "FAIL");
      return;
    }
    if (strcmp(sub, "vel") == 0) {
      if (msg.argc < 4) {
        statusMessage("RC: vel requires endpoint vel accel");
        return;
      }
      uint32_t vel = 0;
      uint32_t accel = 0;
      if (!parseUint32(msg.argv[2], vel) || !parseUint32(msg.argv[3], accel)) {
        statusMessage("RC: vel parse error");
        return;
      }
      const bool ok = _roboclaw.commandVelocity(portIndex, static_cast<uint8_t>(ep->address), ep->motor, vel, accel);
      statusMessage("RC: vel %s", ok ? "OK" : "FAIL");
      return;
    }

    statusMessage("RC: unknown subcommand");
    return;
  }

  if (strcmp(msg.cmd, "can") == 0) {
    if (msg.argc == 0 || strcmp(msg.argv[0], "status") == 0) {
      _can.printHealth(Serial);
      return;
    }
    statusMessage("can status");
    return;
  }

  if (strcmp(msg.cmd, "seq") == 0) {
    if (msg.argc == 0) {
      statusMessage("seq load [path] | seq info");
      return;
    }
    if (strcmp(msg.argv[0], "load") == 0) {
      if (!_sdReady) {
        statusMessage("SEQ: SD not ready");
        return;
      }
      const char *path = (msg.argc > 1) ? msg.argv[1] : SdCardManager::kAnimationFilePath;
      _sequenceLoaded = _sequence.loadFromAnimation(_sd, path, Serial);
      statusMessage("SEQ: load %s", _sequenceLoaded ? "OK" : "FAIL");
      return;
    }
    if (strcmp(msg.argv[0], "info") == 0) {
      Serial.printf("SEQ: %s events=%u loop_ms=%lu\n",
                    _sequenceLoaded ? "LOADED" : "NONE",
                    (unsigned)_sequence.eventCount(),
                    (unsigned long)_sequence.loopMs());
      setStatusLine("SEQ: %s", _sequenceLoaded ? "LOADED" : "NONE");
      return;
    }
    statusMessage("SEQ: unknown subcommand");
    return;
  }

  if (strcmp(msg.cmd, "reboot") == 0) {
    statusMessage("REBOOTING...");
    rebootNow();
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

void App::setStatusConfigSave(bool epOk) {
  setStatusLine(epOk ? "CFG SAVE EP" : "CFG SAVE EEPROM");
}

void App::setStatusConfigReset(bool epOk) {
  setStatusLine(epOk ? "CFG RESET EP" : "CFG RESET EEPROM");
}

void App::setStatusSdTest(bool ok) {
  setStatusLine("SD TEST: %s", ok ? "OK" : "FAIL");
}

void App::setStatusRebooting() {
  setStatusLine("REBOOTING");
}

void App::actionSaveConfig() {
  _configStore.save(_config);
  bool epOk = _sdReady && _sd.saveEndpointConfig(_config, Serial);
  setStatusConfigSave(epOk);
}

void App::actionResetConfig() {
  _configStore.setDefaults(_config);
  _configStore.save(_config);
  bool epOk = _sdReady && _sd.saveEndpointConfig(_config, Serial);
  setStatusConfigReset(epOk);
}

void App::actionFactoryReset() {
  _configStore.setDefaults(_config);
  _configStore.save(_config);
  _configLoaded = true;
  _configFromEndpoints = false;

  bool epOk = false;
  bool animOk = false;
  if (_sdReady) {
    epOk = _sd.saveEndpointConfig(_config, Serial);
    animOk = _sd.saveDefaultAnimation(SdCardManager::kAnimationFilePath, Serial);
    _sequenceLoaded = _sequence.loadFromAnimation(_sd, SdCardManager::kAnimationFilePath, Serial);
    _configFromEndpoints = epOk;
  }

  if (_sdReady) {
    if (epOk && animOk) {
      setStatusLine("FACTORY: SD OK");
    } else if (epOk) {
      setStatusLine("FACTORY: EP OK");
    } else {
      setStatusLine("FACTORY: SD ERR");
    }
  } else {
    setStatusLine("FACTORY: EEPROM");
  }
}

void App::actionOpenEdit() {
  _show.setPlaying(false);
  _model.playing = false;
  stopRoboClaws();
  _editField = 0;
  _editPosTickAccum = 0;
  selectFirstEditEvent(static_cast<uint8_t>(_model.selectedMotor + 1));
  _screen = UiScreen::Edit;
}

void App::actionSdTest() {
  bool ok = _sd.testReadWrite(Serial);
  setStatusSdTest(ok);
}

void App::actionReboot() {
  setStatusRebooting();
  rebootNow();
}
