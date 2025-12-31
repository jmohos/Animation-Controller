#pragma once
#include <Arduino.h>

class EncoderJog {
public:
  void begin(uint8_t pinA, uint8_t pinB);
  int32_t consumeDelta();      // ticks since last call
  int32_t position() const { return _pos; }

private:
  static void isrA();
  static void isrB();
  static inline EncoderJog* _self = nullptr;

  uint8_t _pinA = 255, _pinB = 255;
  volatile int32_t _pos = 0;
  volatile int32_t _last = 0;
};
