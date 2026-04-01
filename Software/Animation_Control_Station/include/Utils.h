#pragma once
#include <Arduino.h>

uint8_t clampU8(int32_t value, uint8_t minValue, uint8_t maxValue);
uint32_t clampU32(int64_t value, uint32_t minValue, uint32_t maxValue);
int32_t clampI32(int64_t value, int32_t minValue, int32_t maxValue);
uint32_t clampU32Range(uint32_t value, uint32_t minValue, uint32_t maxValue);
int32_t clampI32Range(int32_t value, int32_t minValue, int32_t maxValue);

bool parseUint32(const char *text, uint32_t &value);
bool parseInt32(const char *text, int32_t &value);
