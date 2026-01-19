#include "RoboClawBus.h"
#include <cstring>

static int32_t toSigned32(uint32_t value) {
  int32_t signedValue = 0;
  std::memcpy(&signedValue, &value, sizeof(signedValue));
  return signedValue;
}

void RoboClawBus::begin(Rs422Ports &ports, uint32_t timeoutMs, uint32_t baud) {
  for (uint8_t i = 0; i < kMaxPorts; i++) {
    HardwareSerial *serial = ports.port(i).serial;
    if (!serial) {
      _claws[i] = nullptr;
      continue;
    }
    _claws[i] = new (_storage[i]) RoboClaw(serial, timeoutMs);
    _claws[i]->begin(baud);
  }
}

bool RoboClawBus::readStatus(uint8_t portIndex, uint8_t address, RoboClawStatus &status) {
  if (portIndex >= kMaxPorts || !_claws[portIndex]) {
    return false;
  }
  RoboClaw *rc = _claws[portIndex];
  bool anyValid = false;
  bool valid = false;
  status.error = rc->ReadError(address, &valid);
  status.errorValid = valid;
  if (status.errorValid) {
    anyValid = true;
  }
  uint32_t enc1 = 0;
  uint32_t enc2 = 0;
  status.encValid = rc->ReadEncoders(address, enc1, enc2);
  if (status.encValid) {
    status.enc1 = toSigned32(enc1);
    status.enc2 = toSigned32(enc2);
    anyValid = true;
  }
  uint32_t speed1 = 0;
  uint32_t speed2 = 0;
  status.speedValid = rc->ReadISpeeds(address, speed1, speed2);
  if (status.speedValid) {
    status.speed1 = toSigned32(speed1);
    status.speed2 = toSigned32(speed2);
    anyValid = true;
  }
  return anyValid;
}

bool RoboClawBus::commandPosition(uint8_t portIndex, uint8_t address, uint8_t motor, uint32_t position, uint32_t velocity, uint32_t accel) {
  if (portIndex >= kMaxPorts || !_claws[portIndex]) {
    return false;
  }
  RoboClaw *rc = _claws[portIndex];
  const uint32_t deccel = accel;
  const uint8_t flags = 1; // RoboClaw: 1 executes immediately, 0 queues.
  if (motor == 1) {
    return rc->SpeedAccelDeccelPositionM1(address, accel, velocity, deccel, position, flags);
  }
  if (motor == 2) {
    return rc->SpeedAccelDeccelPositionM2(address, accel, velocity, deccel, position, flags);
  }
  return false;
}

bool RoboClawBus::commandVelocity(uint8_t portIndex, uint8_t address, uint8_t motor, uint32_t velocity, uint32_t accel) {
  if (portIndex >= kMaxPorts || !_claws[portIndex]) {
    return false;
  }
  RoboClaw *rc = _claws[portIndex];
  if (motor == 1) {
    return rc->SpeedAccelM1(address, accel, velocity);
  }
  if (motor == 2) {
    return rc->SpeedAccelM2(address, accel, velocity);
  }
  return false;
}
