#pragma once
#include <Arduino.h>
#include <EEPROM.h>

static constexpr uint8_t MAX_ENDPOINTS = 16;

struct EndpointConfig {
  uint8_t serialPort = 1; // 1-8
  uint8_t motor = 1;      // 1=M1, 2=M2
  uint8_t address = 0x80; // RoboClaw address
  uint8_t enabled = 1;
  uint32_t maxVelocity = 50000;
  uint32_t maxAccel = 50000;
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
  static constexpr uint16_t kVersion = 4;
};
