#pragma once
#include <Arduino.h>

// MKS Servo Protocol Constants
namespace MksServo {
  static constexpr uint8_t CMD_POSITION = 0xF5;
  static constexpr uint8_t CMD_VELOCITY = 0xF6;
  static constexpr uint8_t CMD_READ_POSITION = 0x30;
  static constexpr int32_t MAX_POSITION_PULSES = 0x7FFFFF;  // 24-bit signed max
  static constexpr uint16_t MAX_VELOCITY_RPM = 3000;
  static constexpr uint8_t MAX_ACCEL = 255;
  static constexpr uint32_t DEFAULT_PULSES_PER_REV = 16384;  // MKS encoder subdivisions
}

// System Constants
static constexpr uint32_t kMaxVelocityCountsPerSec = 50000;
static constexpr uint32_t kMaxAccelCountsPerSec2 = 50000;
static constexpr uint32_t kMaxTimeMs = 300000;  // 5 minutes
static constexpr uint8_t kCsvFieldCount = 16;  // Updated from 12 to 16

// Clamping Functions
uint8_t clampU8(int32_t value, uint8_t minValue, uint8_t maxValue);
uint32_t clampU32(int64_t value, uint32_t minValue, uint32_t maxValue);
int32_t clampI32(int64_t value, int32_t minValue, int32_t maxValue);
uint32_t clampU32Range(uint32_t value, uint32_t minValue, uint32_t maxValue);
int32_t clampI32Range(int32_t value, int32_t minValue, int32_t maxValue);

// Parsing Functions
bool parseUint32(const char *text, uint32_t &value);
bool parseInt32(const char *text, int32_t &value);
