#pragma once
#include "St7789T4Custom.h"

// Select which panel is attached.
// Default: existing 2.0" 240x320 ST7789 panel.
// - To use the Waveshare 1.69" 240x280 (ST7789V2), define DISPLAY_WAVESHARE_169
// - To use an ILI9341 240x320 with the Teensy-optimized ILI9341_T4 driver, define DISPLAY_ILI9341_T4

#if defined(DISPLAY_ILI9341_T4)
  // ILI9341 is handled separately; no ST7789 resolution needed.
#elif defined(DISPLAY_WAVESHARE_169)
static constexpr St7789Resolution DISPLAY_RESOLUTION = St7789Resolution::ST7789_240x280;
#else
static constexpr St7789Resolution DISPLAY_RESOLUTION = St7789Resolution::ST7789_240x320;
#endif
