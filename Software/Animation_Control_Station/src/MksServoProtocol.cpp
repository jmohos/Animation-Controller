#include "MksServoProtocol.h"

namespace MksServoProtocol {
  uint8_t checksum(uint16_t canId, const uint8_t *data, uint8_t len) {
    // MKS CAN checksum: sum of CAN ID (low byte) + data bytes, truncated to 8 bits
    uint32_t sum = static_cast<uint8_t>(canId & 0xFFu);
    for (uint8_t i = 0; i < len; i++) {
      sum += data[i];
    }
    return static_cast<uint8_t>(sum & 0xFFu);
  }

  uint32_t encodeInt24(int32_t value) {
    // Clamp to 24-bit signed range: ±8388607
    const int32_t maxPos = MksServo::MAX_POSITION_PULSES;
    if (value > maxPos) {
      value = maxPos;
    } else if (value < -maxPos) {
      value = -maxPos;
    }

    // Convert signed to unsigned 24-bit representation
    if (value < 0) {
      return static_cast<uint32_t>((1u << 24) + value);
    }
    return static_cast<uint32_t>(value);
  }

  bool packPosition(uint16_t canId, uint16_t speed, uint8_t accel, int32_t position, uint8_t data[8]) {
    // Validate CAN ID (11-bit standard CAN)
    if (canId > 0x7FFu) {
      return false;
    }

    // Encode position to 24-bit unsigned
    uint32_t axis = encodeInt24(position);

    // Pack command frame
    data[0] = MksServo::CMD_POSITION;
    data[1] = static_cast<uint8_t>((speed >> 8) & 0xFFu);
    data[2] = static_cast<uint8_t>(speed & 0xFFu);
    data[3] = accel;
    data[4] = static_cast<uint8_t>((axis >> 16) & 0xFFu);
    data[5] = static_cast<uint8_t>((axis >> 8) & 0xFFu);
    data[6] = static_cast<uint8_t>(axis & 0xFFu);
    data[7] = checksum(canId, data, 7);

    return true;
  }

  bool packSpeed(uint16_t canId, uint16_t speed, uint8_t accel, bool reverse, uint8_t data[5]) {
    // Validate CAN ID
    if (canId > 0x7FFu) {
      return false;
    }

    // Direction flag in bit 7, speed high nibble in bits 3:0
    const uint8_t dir = reverse ? 0x80u : 0x00u;

    // Pack command frame
    data[0] = MksServo::CMD_VELOCITY;
    data[1] = static_cast<uint8_t>(dir | ((speed >> 8) & 0x0Fu));
    data[2] = static_cast<uint8_t>(speed & 0xFFu);
    data[3] = accel;
    data[4] = checksum(canId, data, 4);

    return true;
  }

  bool parsePositionResponse(const uint8_t *data, uint8_t len, int32_t &position) {
    // Verify minimum length
    if (!data || len < 4) {
      return false;
    }

    // Verify command byte
    if (data[0] != MksServo::CMD_READ_POSITION) {
      return false;
    }

    // Parse 24-bit signed position (bytes 1-3, big-endian)
    int32_t pos = (data[1] << 16) | (data[2] << 8) | data[3];

    // Sign extend from 24-bit to 32-bit
    if (pos & 0x800000) {
      pos |= 0xFF000000;
    }

    position = pos;
    return true;
  }
}
