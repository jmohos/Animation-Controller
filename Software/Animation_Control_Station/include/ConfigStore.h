#pragma once
#include <Arduino.h>
#include <EEPROM.h>
#include "EndpointTypes.h"

static constexpr uint8_t MAX_ENDPOINTS = 16;

struct EndpointConfig {
  EndpointType type = EndpointType::RoboClaw;
  uint8_t serialPort = 0;  // Interface index (0=CAN, 1-7=RS422 ports)
  uint8_t motor = 1;       // RoboClaw motor (1=M1, 2=M2), 0 when unused
  uint32_t address = 0x80; // Device address / CAN ID
  uint8_t enabled = 1;
  int32_t positionMin = 0;
  int32_t positionMax = 0;
  uint32_t velocityMin = 0;
  uint32_t velocityMax = 50000;
  uint32_t accelMin = 0;
  uint32_t accelMax = 50000;
};

struct AppConfig {
  uint32_t magic = 0;
  uint16_t version = 0;
  uint16_t size = 0;
  EndpointConfig endpoints[MAX_ENDPOINTS] = {};
};

class ConfigStore {
public:
  /**
   * Description: Load configuration from EEPROM.
   * Inputs: None.
   * Outputs: Returns true when a valid config is loaded.
   */
  bool load(AppConfig &cfg) const;

  /**
   * Description: Save configuration to EEPROM.
   * Inputs:
   * - cfg: configuration to store.
   * Outputs: Writes the config to EEPROM.
   */
  void save(const AppConfig &cfg) const;

  /**
   * Description: Populate default configuration values.
   * Inputs:
   * - cfg: configuration structure to fill.
   * Outputs: Sets defaults for endpoint mapping and metadata.
   */
  void setDefaults(AppConfig &cfg) const;

private:
  static constexpr uint32_t kMagic = 0x43464731; // "CFG1"
  static constexpr uint16_t kVersion = 6;
};
