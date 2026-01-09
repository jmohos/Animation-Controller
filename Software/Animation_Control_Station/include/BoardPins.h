#pragma once
#include <Arduino.h>

// ---------------- LCD: Waveshare 2" ST7789V 240x320 ----------------
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
  BUTTON_OK     = 0,
  BUTTON_DOWN   = 1,
  BUTTON_UP     = 2,
  BUTTON_LEFT   = 3,
  BUTTON_RIGHT  = 4,
  BUTTON_RED    = 5,
  BUTTON_YELLOW = 6,
  BUTTON_GREEN  = 7,

  LED_Red       = 8,
  LED_Yellow    = 9,
  LED_Green     = 10,
  LED_SPARE_1   = 11,
  LED_SPARE_2   = 12,
  LED_SPARE_3   = 13,
  LED_SPARE_4   = 14,
  LED_SPARE_5   = 15,

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
