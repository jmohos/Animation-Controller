#include "ConfigStore.h"

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
    EndpointConfig &ep = cfg.endpoints[i];
    ep.serialPort = 1;
    ep.motor = (i % 2) + 1;
    ep.address = 0x80;
    ep.enabled = (i < 2);
    ep.maxVelocity = 3000;
    ep.maxAccel = 6000;
  }
}
