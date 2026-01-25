#pragma once
#include <Arduino.h>
#include <elapsedMillis.h>
#include "ConfigStore.h"
#include "BoardPins.h"

static constexpr uint8_t MAX_RC_PORTS = RS422_PORT_COUNT;

struct UiModel {
  enum class Screen : uint8_t {
    Boot,
    Manual,
    Auto,
    Edit,
    Menu,
    Settings,
    Diagnostics,
    Endpoints,
    EndpointConfig,
    EndpointConfigEdit,
    RoboClawStatus
  };

  Screen screen = Screen::Boot;
  bool playing = false;
  uint32_t showTimeMs = 0;
  uint8_t selectedMotor = 0;
  float speedNorm = 0.0f;
  float accelNorm = 0.0f;
  int32_t jogPos = 0;
  uint8_t menuIndex = 0;
  uint8_t settingsIndex = 0;
  uint8_t diagnosticsIndex = 0;
  uint8_t endpointConfigIndex = 0;
  uint8_t endpointConfigField = 0;
  bool endpointConfigEditing = false;
  bool sdReady = false;
  char statusLine[32] = {};
  bool rcStatusValid = false;
  bool rcEncValid = false;
  bool rcSpeedValid = false;
  bool rcErrorValid = false;
  int32_t rcEnc1 = 0;
  int32_t rcEnc2 = 0;
  int32_t rcSpeed1 = 0;
  int32_t rcSpeed2 = 0;
  uint32_t rcError = 0;
  int32_t rcSelectedEnc = 0;
  int32_t rcSelectedSpeed = 0;
  bool sequenceLoaded = false;
  uint16_t sequenceCount = 0;
  uint32_t sequenceLoopMs = 0;
  bool editHasEvent = false;
  uint16_t editEventOrdinal = 0;
  uint16_t editEventCount = 0;
  uint32_t editTimeMs = 0;
  int32_t editPosition = 0;
  uint32_t editVelocity = 0;
  uint32_t editAccel = 0;
  uint8_t editField = 0;
  bool endpointEnabled[MAX_ENDPOINTS] = {};
  bool endpointStatusValid[MAX_ENDPOINTS] = {};
  bool endpointEncValid[MAX_ENDPOINTS] = {};
  bool endpointSpeedValid[MAX_ENDPOINTS] = {};
  int32_t endpointPos[MAX_ENDPOINTS] = {};
  int32_t endpointSpeed[MAX_ENDPOINTS] = {};
  EndpointType endpointConfigType[MAX_ENDPOINTS] = {};
  uint8_t endpointConfigPort[MAX_ENDPOINTS] = {};
  uint8_t endpointConfigMotor[MAX_ENDPOINTS] = {};
  uint32_t endpointConfigAddress[MAX_ENDPOINTS] = {};
  EndpointConfig endpointConfigSelected = {};
  bool rcPortEnabled[MAX_RC_PORTS] = {};
  uint8_t rcPortAddress[MAX_RC_PORTS] = {};
  bool rcPortStatusValid[MAX_RC_PORTS] = {};
  bool rcPortEncValid[MAX_RC_PORTS] = {};
  bool rcPortSpeedValid[MAX_RC_PORTS] = {};
  bool rcPortErrorValid[MAX_RC_PORTS] = {};
  int32_t rcPortEnc1[MAX_RC_PORTS] = {};
  int32_t rcPortEnc2[MAX_RC_PORTS] = {};
  int32_t rcPortSpeed1[MAX_RC_PORTS] = {};
  int32_t rcPortSpeed2[MAX_RC_PORTS] = {};
  uint32_t rcPortError[MAX_RC_PORTS] = {};
};

using UiScreen = UiModel::Screen;

enum class EndpointField : uint8_t {
  Enabled,
  Type,
  Address,
  SerialPort,
  Motor,
  PositionMin,
  PositionMax,
  VelocityMin,
  VelocityMax,
  AccelMin,
  AccelMax,
  Count
};

static constexpr uint8_t ENDPOINT_FIELD_COUNT = static_cast<uint8_t>(EndpointField::Count);

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
