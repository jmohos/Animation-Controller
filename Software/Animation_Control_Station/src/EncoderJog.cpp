#include "EncoderJog.h"

static inline int readAB(uint8_t a, uint8_t b) {
  return (digitalReadFast(a) ? 2 : 0) | (digitalReadFast(b) ? 1 : 0);
}

void EncoderJog::begin(uint8_t pinA, uint8_t pinB) {
  _self = this;
  _pinA = pinA;
  _pinB = pinB;
  pinMode(_pinA, INPUT_PULLUP);
  pinMode(_pinB, INPUT_PULLUP);
  _last = readAB(_pinA, _pinB);

  attachInterrupt(digitalPinToInterrupt(_pinA), EncoderJog::isrA, CHANGE);
  attachInterrupt(digitalPinToInterrupt(_pinB), EncoderJog::isrB, CHANGE);
}

void EncoderJog::isrA() {
  if (!_self) return;
  const int now = readAB(_self->_pinA, _self->_pinB);
  // Gray-code style decode
  const int delta = (( _self->_last == 0 && now == 1) ||
                     (_self->_last == 1 && now == 3) ||
                     (_self->_last == 3 && now == 2) ||
                     (_self->_last == 2 && now == 0)) ? +1 :
                    (( _self->_last == 0 && now == 2) ||
                     (_self->_last == 2 && now == 3) ||
                     (_self->_last == 3 && now == 1) ||
                     (_self->_last == 1 && now == 0)) ? -1 : 0;
  _self->_pos += delta;
  _self->_last = now;
}

void EncoderJog::isrB() { isrA(); }

int32_t EncoderJog::consumeDelta() {
  static int32_t lastRead = 0;
  noInterrupts();
  int32_t p = _pos;
  interrupts();
  int32_t d = p - lastRead;
  lastRead = p;
  return d;
}
