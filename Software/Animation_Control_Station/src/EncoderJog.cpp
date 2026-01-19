#include "EncoderJog.h"

/**
 * Description: Read the current A/B pin state and encode it.
 * Inputs:
 * - pinA: encoder channel A pin.
 * - pinB: encoder channel B pin.
 * Outputs: Returns a 2-bit encoded state.
 */
static inline int readAB(uint8_t pinA, uint8_t pinB) {
  return (digitalReadFast(pinA) ? 2 : 0) | (digitalReadFast(pinB) ? 1 : 0);
}

/**
 * Description: Initialize the encoder pins and attach interrupts.
 * Inputs:
 * - pinA: encoder channel A pin.
 * - pinB: encoder channel B pin.
 * Outputs: Configures pin modes and interrupt handlers.
 */
void EncoderJog::begin(uint8_t pinA, uint8_t pinB) {
  _self = this;
  _pinA = pinA;
  _pinB = pinB;
  pinMode(_pinA, INPUT_PULLUP);
  pinMode(_pinB, INPUT_PULLUP);
  _lastState = readAB(_pinA, _pinB);

  attachInterrupt(digitalPinToInterrupt(_pinA), EncoderJog::isrA, CHANGE);
  attachInterrupt(digitalPinToInterrupt(_pinB), EncoderJog::isrB, CHANGE);
}

/**
 * Description: ISR for channel A that updates the encoder position.
 * Inputs: None (ISR context).
 * Outputs: Updates internal position and last state.
 */
void EncoderJog::isrA() {
  if (!_self) return;
  const int now = readAB(_self->_pinA, _self->_pinB);
  // Gray-code style decode
  const int delta = (( _self->_lastState == 0 && now == 1) ||
                     (_self->_lastState == 1 && now == 3) ||
                     (_self->_lastState == 3 && now == 2) ||
                     (_self->_lastState == 2 && now == 0)) ? +1 :
                    (( _self->_lastState == 0 && now == 2) ||
                     (_self->_lastState == 2 && now == 3) ||
                     (_self->_lastState == 3 && now == 1) ||
                     (_self->_lastState == 1 && now == 0)) ? -1 : 0;
  _self->_position += delta;
  _self->_lastState = now;
}

/**
 * Description: ISR for channel B that delegates to channel A logic.
 * Inputs: None (ISR context).
 * Outputs: Updates internal position and last state.
 */
void EncoderJog::isrB() { isrA(); }

/**
 * Description: Return and clear the delta since the last read.
 * Inputs: None.
 * Outputs: Returns the delta tick count since last call.
 */
int32_t EncoderJog::consumeDelta() {
  static int32_t lastReadPosition = 0;
  static int32_t tickRemainder = 0;
  noInterrupts();
  int32_t position = _position;
  interrupts();
  int32_t delta = position - lastReadPosition;
  lastReadPosition = position;
  tickRemainder += delta;
  const int32_t ticksPerDetent = 4;
  const int32_t detents = tickRemainder / ticksPerDetent;
  tickRemainder -= detents * ticksPerDetent;
  return detents;
}
