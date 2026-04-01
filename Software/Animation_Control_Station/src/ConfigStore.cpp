#include "ConfigStore.h"

namespace {
void clearEndpoint(EndpointConfig &ep, uint8_t stationId) {
  ep.type = EndpointType::AnimComOctal;
  ep.stationId = stationId;
  ep.enabled = 0;
  ep.itemCount = 0;
  for (uint8_t i = 0; i < MAX_ENDPOINT_ITEMS; i++) {
    ep.itemTypes[i] = EndpointItemType::None;
  }
}
}

bool ConfigStore::load(AppConfig &cfg) const {
  EEPROM.get(0, cfg);
  if (cfg.magic != kMagic) {
    return false;
  }
  if (cfg.version != kVersion) {
    return false;
  }
  if (cfg.size != sizeof(AppConfig)) {
    return false;
  }
  return true;
}

void ConfigStore::save(const AppConfig &cfg) const {
  EEPROM.put(0, cfg);
}

void ConfigStore::setDefaults(AppConfig &cfg) const {
  cfg.magic = kMagic;
  cfg.version = kVersion;
  cfg.size = sizeof(AppConfig);
  for (uint8_t i = 0; i < MAX_ENDPOINTS; i++) {
    clearEndpoint(cfg.endpoints[i], static_cast<uint8_t>(i + 1));
  }
}
