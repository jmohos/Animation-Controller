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
    ep.pulsesPerRevolution = 0;
    ep.homeOffset = 0;
    ep.homeDirection = 0;
    ep.hasLimitSwitch = 0;
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
  ep1.pulsesPerRevolution = 16384;  // MKS Servo encoder subdivisions (16384/rev)
  ep1.homeOffset = 0;
  ep1.homeDirection = 0;
  ep1.hasLimitSwitch = 0;
  ep1.positionMin = -360;   // Engineering units: degrees
  ep1.positionMax = 360;
  ep1.velocityMin = 0;
  ep1.velocityMax = 100;    // Engineering units: deg/s
  ep1.accelMin = 0;
  ep1.accelMax = 50;        // Engineering units: deg/s²

  EndpointConfig &ep2 = cfg.endpoints[1];
  ep2.type = EndpointType::MksServo;
  ep2.serialPort = 0;
  ep2.motor = 0;
  ep2.address = 2;
  ep2.enabled = 1;
  ep2.pulsesPerRevolution = 16384;  // MKS Servo encoder subdivisions (16384/rev)
  ep2.homeOffset = 0;
  ep2.homeDirection = 0;
  ep2.hasLimitSwitch = 0;
  ep2.positionMin = -720;   // Engineering units: degrees (2 revolutions)
  ep2.positionMax = 720;
  ep2.velocityMin = 0;
  ep2.velocityMax = 100;    // Engineering units: deg/s
  ep2.accelMin = 0;
  ep2.accelMax = 50;        // Engineering units: deg/s²

  EndpointConfig &ep3 = cfg.endpoints[2];
  ep3.type = EndpointType::RoboClaw;
  ep3.serialPort = 1;
  ep3.motor = 1;
  ep3.address = 0x80;
  ep3.enabled = 1;
  ep3.pulsesPerRevolution = 4096;  // Common encoder resolution
  ep3.homeOffset = 0;
  ep3.homeDirection = 0;
  ep3.hasLimitSwitch = 0;
  ep3.positionMin = -360;   // Engineering units: degrees
  ep3.positionMax = 360;
  ep3.velocityMin = 0;
  ep3.velocityMax = 100;    // Engineering units: deg/s
  ep3.accelMin = 0;
  ep3.accelMax = 50;        // Engineering units: deg/s²

  EndpointConfig &ep4 = cfg.endpoints[3];
  ep4.type = EndpointType::RoboClaw;
  ep4.serialPort = 1;
  ep4.motor = 2;
  ep4.address = 0x80;
  ep4.enabled = 1;
  ep4.pulsesPerRevolution = 4096;  // Common encoder resolution
  ep4.homeOffset = 0;
  ep4.homeDirection = 0;
  ep4.hasLimitSwitch = 0;
  ep4.positionMin = -360;   // Engineering units: degrees
  ep4.positionMax = 360;
  ep4.velocityMin = 0;
  ep4.velocityMax = 100;    // Engineering units: deg/s
  ep4.accelMin = 0;
  ep4.accelMax = 50;        // Engineering units: deg/s²
}
