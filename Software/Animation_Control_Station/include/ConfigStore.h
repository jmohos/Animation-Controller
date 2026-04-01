#pragma once
#include <Arduino.h>
#include <EEPROM.h>
#include "EndpointTypes.h"

static constexpr uint8_t MAX_ENDPOINTS = 16;
static constexpr uint8_t MAX_ENDPOINT_ITEMS = 8;

struct EndpointConfig {
  EndpointType type = EndpointType::AnimComOctal;
  uint8_t stationId = 1;
  uint8_t enabled = 0;
  uint8_t itemCount = 0;
  EndpointItemType itemTypes[MAX_ENDPOINT_ITEMS] = {};
};

struct AppConfig {
  uint32_t magic = 0;
  uint16_t version = 0;
  uint16_t size = 0;
  EndpointConfig endpoints[MAX_ENDPOINTS] = {};
};

class ConfigStore {
public:
  bool load(AppConfig &cfg) const;
  void save(const AppConfig &cfg) const;
  void setDefaults(AppConfig &cfg) const;

private:
  static constexpr uint32_t kMagic = 0x43464731;
  static constexpr uint16_t kVersion = 9;
};
