#pragma once
#include <Arduino.h>

class EncoderJog {
public:
  /**
   * Description: Initialize the encoder pins and attach interrupts.
   * Inputs:
   * - pinA: encoder channel A pin.
   * - pinB: encoder channel B pin.
   * Outputs: Configures GPIO and ISR hookups.
   */
  void begin(uint8_t pinA, uint8_t pinB);

  /**
   * Description: Consume and return ticks accumulated since last call.
   * Inputs: None.
   * Outputs: Returns delta ticks since the previous call.
   */
  int32_t consumeDelta();      // ticks since last call

  /**
   * Description: Get the absolute encoder position.
   * Inputs: None.
   * Outputs: Returns the current tick position.
   */
  int32_t position() const { return _position; }

private:
  /**
   * Description: ISR for channel A transitions (updates position).
   * Inputs: None (ISR context).
   * Outputs: Updates encoder position state.
   */
  static void isrA();

  /**
   * Description: ISR for channel B transitions (shares decoder with A).
   * Inputs: None (ISR context).
   * Outputs: Updates encoder position state.
   */
  static void isrB();
  static inline EncoderJog* _self = nullptr;

  uint8_t _pinA = 255, _pinB = 255;
  volatile int32_t _position = 0;
  volatile int32_t _lastState = 0;
};
