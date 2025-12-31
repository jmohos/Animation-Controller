#pragma once
#include <Arduino.h>

#ifndef LOG_LEVEL
#define LOG_LEVEL 2
#endif

inline void logInit(unsigned long baud = 115200) {
  Serial.begin(baud);
  // Don’t block forever if USB isn’t connected
  const uint32_t t0 = millis();
  while (!Serial && (millis() - t0) < 500) {}
}

#if LOG_LEVEL >= 1
  #define LOGI(...) do { Serial.printf("[I] " __VA_ARGS__); Serial.print("\n"); } while(0)
#else
  #define LOGI(...) do {} while(0)
#endif

#if LOG_LEVEL >= 2
  #define LOGD(...) do { Serial.printf("[D] " __VA_ARGS__); Serial.print("\n"); } while(0)
#else
  #define LOGD(...) do {} while(0)
#endif

#if LOG_LEVEL >= 3
  #define LOGV(...) do { Serial.printf("[V] " __VA_ARGS__); Serial.print("\n"); } while(0)
#else
  #define LOGV(...) do {} while(0)
#endif
