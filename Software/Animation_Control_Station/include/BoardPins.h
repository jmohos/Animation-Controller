#pragma once
#include <Arduino.h>

// ---------------- LCD: ILI9341 240x320 ----------------
// Teensy 4.1 SPI0: MOSI=11, MISO=12, SCK=13 (hardware)
static constexpr uint8_t PIN_LCD_CS   = 10;
static constexpr uint8_t PIN_LCD_DC   = 39;
// Drive reset from GPIO38; if your module lacks RST, set to 255 (unused)
static constexpr uint8_t PIN_LCD_RST  = 38;
static constexpr uint8_t PIN_LCD_BL   = 255;   // optional backlight control

// ---------------- I2C: SX1509 ----------------
static constexpr uint8_t PIN_I2C_SDA      = 18;
static constexpr uint8_t PIN_I2C_SCL      = 19;
static constexpr uint8_t PIN_SX1509_INT   = 22;

// SX1509 pin assignments (P0..P15)
enum class SXPin : uint8_t {
  SX_BUTTON_1  = 0,
  SX_BUTTON_2  = 1,
  SX_BUTTON_3  = 2,
  SX_BUTTON_4  = 3,
  SX_BUTTON_5  = 4,
  SX_BUTTON_6  = 5,
  SX_BUTTON_7  = 6,
  SX_BUTTON_8  = 7,

  SX_LED_1     = 8,
  SX_LED_2     = 9,
  SX_LED_3     = 10,
  SX_LED_4     = 11,
  SX_LED_5     = 12,
  SX_LED_6     = 13,
  SX_LED_7     = 14,
  SX_LED_8     = 15,

  COUNT
};
static constexpr uint8_t SX1509_PIN_COUNT = static_cast<uint8_t>(SXPin::COUNT);

// ---------------- Analog pots ----------------
static constexpr uint8_t PIN_POT_SPEED = A16;
static constexpr uint8_t PIN_POT_ACCEL = A17;

// ---------------- Quadrature jog encoder ----------------
static constexpr uint8_t PIN_ENC_A = 5;
static constexpr uint8_t PIN_ENC_B = 4;

// ---------------- RS485 RoboClaw ports ----------------
struct Rs422PortPins {
  uint8_t rx;
  uint8_t tx;
};

static constexpr Rs422PortPins ROBOCLAW_PORT_PINS[8] = {
  /*1*/ {0,  1},
  /*2*/ {7,  8},
  /*3*/ {15, 14},
  /*4*/ {16, 17},
  /*5*/ {21, 20},
  /*6*/ {25, 24},
  /*7*/ {28, 29},
  /*8*/ {34, 35},
};
