#pragma once
#include <Arduino.h>
#include <new>
#include "Rs422Ports.h"
#include <RoboClaw.h>

struct RoboClawStatus {
  int32_t enc1 = 0;
  int32_t enc2 = 0;
  bool encValid = false;
  int32_t speed1 = 0;
  int32_t speed2 = 0;
  bool speedValid = false;
  uint32_t error = 0;
  bool errorValid = false;
};

class RoboClawBus {
public:
  /**
   * Description: Initialize RoboClaw drivers for each RS422 port.
   * Inputs:
   * - ports: RS422 port collection.
   * - timeoutMs: serial timeout for RoboClaw transactions.
   * - baud: baud rate for the serial ports.
   * Outputs: Instantiates RoboClaw drivers and configures baud.
   */
  void begin(Rs422Ports &ports, uint32_t timeoutMs, uint32_t baud);

  /**
   * Description: Read encoder and error status from a RoboClaw.
   * Inputs:
   * - portIndex: RS422 port index [0..7].
   * - address: RoboClaw address.
   * - status: output status structure.
   * Outputs: Returns true when any status field is valid.
   */
  bool readStatus(uint8_t portIndex, uint8_t address, RoboClawStatus &status);

  /**
   * Description: Command position with accel and velocity.
   * Inputs:
   * - portIndex: RS422 port index [0..7].
   * - address: RoboClaw address.
   * - motor: motor index (1=M1, 2=M2).
   * - position: target position (encoder counts).
   * - velocity: max velocity (counts/sec).
   * - accel: max accel (counts/sec^2).
   * Outputs: Returns true on successful command.
   */
  bool commandPosition(uint8_t portIndex, uint8_t address, uint8_t motor, uint32_t position, uint32_t velocity, uint32_t accel);

  /**
   * Description: Command velocity with accel.
   * Inputs:
   * - portIndex: RS422 port index [0..7].
   * - address: RoboClaw address.
   * - motor: motor index (1=M1, 2=M2).
   * - velocity: max velocity (counts/sec).
   * - accel: max accel (counts/sec^2).
   * Outputs: Returns true on successful command.
   */
  bool commandVelocity(uint8_t portIndex, uint8_t address, uint8_t motor, uint32_t velocity, uint32_t accel);

private:
  static constexpr uint8_t kMaxPorts = 8;
  RoboClaw* _claws[kMaxPorts] = {};
  alignas(RoboClaw) uint8_t _storage[kMaxPorts][sizeof(RoboClaw)] = {};
};
