#pragma once
#include <Arduino.h>
#include "Utils.h"

/**
 * MKS Servo CAN Protocol Functions
 *
 * Provides encoding/decoding for MKS Servo CAN bus commands.
 * Protocol documentation: MKS Servo CAN Bus Manual
 *
 * Position Command (0xF5): 8 bytes
 *   [0] = 0xF5 (command)
 *   [1:2] = speed (16-bit, 0-3000 RPM)
 *   [3] = accel (8-bit, 0-255 scale)
 *   [4:6] = position (24-bit signed, pulses)
 *   [7] = checksum
 *
 * Velocity Command (0xF6): 5 bytes
 *   [0] = 0xF6 (command)
 *   [1] = direction_flag (0x80=reverse) | speed_high (4 bits)
 *   [2] = speed_low (8 bits)
 *   [3] = accel (8-bit)
 *   [4] = checksum
 *
 * Read Position Command (0x30): 1 byte
 *   [0] = 0x30
 */
namespace MksServoProtocol {
  /**
   * Calculate MKS Servo checksum.
   * Checksum = sum of CAN ID (low byte) + all data bytes (before checksum field)
   * @param canId CAN identifier
   * @param data Payload bytes (before checksum)
   * @param len Number of bytes to checksum
   * @return 8-bit checksum value
   */
  uint8_t checksum(uint16_t canId, const uint8_t *data, uint8_t len);

  /**
   * Encode signed 32-bit position to 24-bit for MKS Servo.
   * Clamps to ±8388607 (24-bit signed max).
   * @param value Signed position
   * @return 24-bit unsigned representation
   */
  uint32_t encodeInt24(int32_t value);

  /**
   * Pack MKS Servo position command (0xF5).
   * @param canId CAN identifier
   * @param speed Velocity in RPM (0-3000)
   * @param accel Acceleration scale (0-255)
   * @param position Target position in pulses (24-bit signed)
   * @param data Output buffer (8 bytes)
   * @return true on success
   */
  bool packPosition(uint16_t canId, uint16_t speed, uint8_t accel, int32_t position, uint8_t data[8]);

  /**
   * Pack MKS Servo velocity command (0xF6).
   * @param canId CAN identifier
   * @param speed Velocity in RPM (0-3000)
   * @param accel Acceleration scale (0-255)
   * @param reverse Direction flag (false=forward, true=reverse)
   * @param data Output buffer (5 bytes)
   * @return true on success
   */
  bool packSpeed(uint16_t canId, uint16_t speed, uint8_t accel, bool reverse, uint8_t data[5]);

  /**
   * Parse MKS Servo position response (0x30 response).
   * @param data Input buffer (response payload)
   * @param len Buffer length (must be >= 4)
   * @param position Output position in pulses
   * @return true if parsed successfully
   */
  bool parsePositionResponse(const uint8_t *data, uint8_t len, int32_t &position);
}
