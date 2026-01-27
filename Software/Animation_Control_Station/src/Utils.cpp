#include "Utils.h"
#include <cstdlib>

uint8_t clampU8(int32_t value, uint8_t minValue, uint8_t maxValue) {
  if (value < minValue) {
    return minValue;
  }
  if (value > maxValue) {
    return maxValue;
  }
  return static_cast<uint8_t>(value);
}

uint32_t clampU32(int64_t value, uint32_t minValue, uint32_t maxValue) {
  if (value < static_cast<int64_t>(minValue)) {
    return minValue;
  }
  if (value > static_cast<int64_t>(maxValue)) {
    return maxValue;
  }
  return static_cast<uint32_t>(value);
}

int32_t clampI32(int64_t value, int32_t minValue, int32_t maxValue) {
  if (value < static_cast<int64_t>(minValue)) {
    return minValue;
  }
  if (value > static_cast<int64_t>(maxValue)) {
    return maxValue;
  }
  return static_cast<int32_t>(value);
}

uint32_t clampU32Range(uint32_t value, uint32_t minValue, uint32_t maxValue) {
  if (maxValue > 0) {
    if (maxValue < minValue) {
      minValue = maxValue;
    }
    if (value < minValue) {
      return minValue;
    }
    if (value > maxValue) {
      return maxValue;
    }
  } else if (minValue > 0 && value < minValue) {
    return minValue;
  }
  return value;
}

int32_t clampI32Range(int32_t value, int32_t minValue, int32_t maxValue) {
  if (maxValue > minValue) {
    if (value < minValue) {
      return minValue;
    }
    if (value > maxValue) {
      return maxValue;
    }
  }
  return value;
}

bool parseUint32(const char *text, uint32_t &value) {
  if (!text) {
    return false;
  }
  int base = 10;
  if (text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) {
    base = 16;
  }
  char *end = nullptr;
  value = strtoul(text, &end, base);
  return (end != text);
}

bool parseInt32(const char *text, int32_t &value) {
  if (!text) {
    return false;
  }
  int base = 10;
  if (text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) {
    base = 16;
  }
  char *end = nullptr;
  long parsed = strtol(text, &end, base);
  if (end == text) {
    return false;
  }
  value = static_cast<int32_t>(parsed);
  return true;
}
