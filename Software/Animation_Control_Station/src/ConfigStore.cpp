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
    ep.type = EndpointType::RoboClaw;
    ep.serialPort = 0;
    ep.motor = 0;
    ep.address = 0;
    ep.enabled = 0;
    ep.positionMin = 0;
    ep.positionMax = 0;
    ep.velocityMin = 0;
    ep.velocityMax = 0;
    ep.accelMin = 0;
    ep.accelMax = 0;
  }

  // Default endpoints: two CAN MKS servos, then a dual-motor RoboClaw on Serial2 (port 1).
  EndpointConfig &ep1 = cfg.endpoints[0];
  ep1.type = EndpointType::MksServo;
  ep1.serialPort = 0;
  ep1.motor = 0;
  ep1.address = 1;
  ep1.enabled = 1;
  ep1.positionMin = -1000;
  ep1.positionMax = 1000;
  ep1.velocityMin = 0;
  ep1.velocityMax = 1000;
  ep1.accelMin = 0;
  ep1.accelMax = 1000;

  EndpointConfig &ep2 = cfg.endpoints[1];
  ep2.type = EndpointType::MksServo;
  ep2.serialPort = 0;
  ep2.motor = 0;
  ep2.address = 2;
  ep2.enabled = 1;
  ep2.positionMin = -1000;
  ep2.positionMax = 1000;
  ep2.velocityMin = 0;
  ep2.velocityMax = 1000;
  ep2.accelMin = 0;
  ep2.accelMax = 1000;

  EndpointConfig &ep3 = cfg.endpoints[2];
  ep3.type = EndpointType::RoboClaw;
  ep3.serialPort = 1;
  ep3.motor = 1;
  ep3.address = 0x80;
  ep3.enabled = 1;
  ep3.positionMin = -1000;
  ep3.positionMax = 1000;
  ep3.velocityMin = 0;
  ep3.velocityMax = 1000;
  ep3.accelMin = 0;
  ep3.accelMax = 1000;

  EndpointConfig &ep4 = cfg.endpoints[3];
  ep4.type = EndpointType::RoboClaw;
  ep4.serialPort = 1;
  ep4.motor = 2;
  ep4.address = 0x80;
  ep4.enabled = 1;
  ep4.positionMin = -1000;
  ep4.positionMax = 1000;
  ep4.velocityMin = 0;
  ep4.velocityMax = 1000;
  ep4.accelMin = 0;
  ep4.accelMax = 1000;
}
