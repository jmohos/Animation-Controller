#pragma once
#include <Arduino.h>

/**
 * Description: Initialize the shared SX1509 device (once).
 * Inputs: None.
 * Outputs: Returns true when SX1509 is ready, false on init failure.
 */
bool sx1509EnsureReady();

/**
 * Description: Check whether the SX1509 device is ready.
 * Inputs: None.
 * Outputs: Returns true when initialized successfully.
 */
bool sx1509Ready();

/**
 * Description: Configure debounce time on the SX1509.
 * Inputs:
 * - timeMs: debounce interval in milliseconds.
 * Outputs: Updates device debounce time.
 */
void sx1509DebounceTime(uint8_t timeMs);

/**
 * Description: Configure a pin mode on the SX1509.
 * Inputs:
 * - pin: SX1509 pin index.
 * - mode: Arduino pin mode (INPUT/INPUT_PULLUP/OUTPUT).
 * Outputs: Updates pin mode.
 */
void sx1509PinMode(uint8_t pin, uint8_t mode);

/**
 * Description: Enable debounce on an SX1509 pin.
 * Inputs:
 * - pin: SX1509 pin index.
 * Outputs: Enables debounce.
 */
void sx1509DebouncePin(uint8_t pin);

/**
 * Description: Read a digital input from the SX1509.
 * Inputs:
 * - pin: SX1509 pin index.
 * Outputs: Returns HIGH or LOW.
 */
int sx1509DigitalRead(uint8_t pin);

/**
 * Description: Initialize an LED driver pin on the SX1509.
 * Inputs:
 * - pin: SX1509 pin index.
 * Outputs: Configures LED driver settings.
 */
void sx1509LedDriverInit(uint8_t pin);

/**
 * Description: Write a PWM value to an SX1509 LED driver pin.
 * Inputs:
 * - pin: SX1509 pin index.
 * - value: PWM duty (0-255).
 * Outputs: Updates LED output.
 */
void sx1509AnalogWrite(uint8_t pin, uint8_t value);
