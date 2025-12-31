#pragma once
#include <stdint.h>
#include <lcd_spi_driver_t4.hpp>

// Project-local fork of the st7789_t4 driver so we can choose a custom
// SPI clock without touching files under .pio/libdeps. Adjust the value
// below (e.g., 10'000'000 for 10 MHz) if your wiring can support it.
// Supports both the original 240x320 panel and the Waveshare 1.69" 240x280 (ST7789V2);
// set the desired resolution in DisplayConfig.h.
static constexpr uint32_t ST7789_SPI_HZ_DEFAULT = 10'000'000;

enum class St7789Resolution : uint8_t {
  ST7789_240x320 = 0,
  ST7789_240x240 = 1,
  ST7789_135x240 = 2,
  ST7789_280x240 = 3,
  ST7789_240x280 = 4,
  ST7789_170x240 = 5,
  ST7789_172x240 = 6
};

class St7789T4Custom : public lcd_spi_driver_t4 {
 public:
  St7789T4Custom(St7789Resolution resolution, uint8_t CS, uint8_t RS,
                 uint8_t SID, uint8_t SCLK, uint8_t RST, uint8_t BKL = 0xFF,
                 uint32_t spi_hz = ST7789_SPI_HZ_DEFAULT);
  St7789T4Custom(St7789Resolution resolution, uint8_t CS, uint8_t RS,
                 uint8_t RST, uint8_t BKL = 0xFF,
                 uint32_t spi_hz = ST7789_SPI_HZ_DEFAULT);

  uint16_t width() const;
  uint16_t height() const;

 protected:
  void initialize(void) override;
  void write_address_window(int x1, int y1, int x2, int y2) override;
  void set_rotation(int rotation) override;

 private:
  void set_dimensions(St7789Resolution resolution);

  uint16_t _native_width = 0;
  uint16_t _native_height = 0;
  uint16_t _offset_x = 0;
  uint16_t _offset_y = 0;
  uint8_t _bkl = 0xFF;
};
