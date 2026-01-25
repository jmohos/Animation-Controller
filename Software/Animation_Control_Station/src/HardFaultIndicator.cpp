#include <Arduino.h>

namespace {
constexpr uint8_t kHardFaultPin = 33;
}

extern "C" void HardFault_Handler(void) __attribute__((noreturn));

extern "C" void HardFault_Handler(void) {
  pinMode(kHardFaultPin, OUTPUT);

  while (true) {
    digitalWriteFast(kHardFaultPin, HIGH);
    for (volatile uint32_t i = 0; i < 3000000u; ++i) {
      __asm__ volatile("nop");
    }
    digitalWriteFast(kHardFaultPin, LOW);
    for (volatile uint32_t i = 0; i < 3000000u; ++i) {
      __asm__ volatile("nop");
    }
  }
}
