#pragma once

#include <Arduino.h>
#include <ILI9341_T4.h>

// Minimal wrapper adding canvas helpers used by the demosauce example.
class ILI9341Wrapper : public ILI9341_T4::ILI9341Driver {
 public:
  /**
   * Description: Construct an ILI9341 wrapper with optional touch pins.
   * Inputs:
   * - cs, dc, sclk, mosi, miso: SPI control pins.
   * - rst: reset pin (255 if unused).
   * - touch_cs: touch chip select (255 if unused).
   * - touch_irq: touch interrupt pin (255 if unused).
   * Outputs: Initializes the base driver with the provided pins.
   */
  ILI9341Wrapper(uint8_t cs, uint8_t dc, uint8_t sclk, uint8_t mosi, uint8_t miso,
                 uint8_t rst = 255, uint8_t touch_cs = 255, uint8_t touch_irq = 255)
      : ILI9341Driver(cs, dc, sclk, mosi, miso, rst, touch_cs, touch_irq) {}

  /**
   * Description: Assign the canvas buffer and dimensions.
   * Inputs:
   * - fb: frame buffer pointer.
   * - width: canvas width in pixels.
   * - height: canvas height in pixels.
   * Outputs: Stores buffer and dimension metadata.
   */
  void setCanvas(uint16_t* fb, int width, int height) {
    _buffer = fb;
    _width = width;
    _height = height;
    _stride = width;
  }

  /**
   * Description: Draw a pixel if it is within bounds.
   * Inputs:
   * - x, y: pixel coordinates.
   * - color: 16-bit RGB565 color.
   * Outputs: Writes the pixel into the canvas buffer.
   */
  inline void drawPixel(int x, int y, uint16_t color) {
    if ((x < 0) || (y < 0) || (x >= _width) || (y >= _height)) return;
    _buffer[x + _stride * y] = color;
  }

  /**
   * Description: Read a pixel if it is within bounds.
   * Inputs:
   * - x, y: pixel coordinates.
   * Outputs: Returns the pixel color, or 0 if out of bounds.
   */
  inline uint16_t readPixel(int x, int y) {
    if ((x < 0) || (y < 0) || (x >= _width) || (y >= _height)) return 0;
    return _buffer[x + _stride * y];
  }

  /**
   * Description: Fill the entire canvas with a single color.
   * Inputs:
   * - color: 16-bit RGB565 color.
   * Outputs: Updates the canvas buffer.
   */
  void fillScreen(uint16_t color) { fillRect(0, 0, _width, _height, color); }

  /**
   * Description: Fill a rectangle with a solid color.
   * Inputs:
   * - x, y: top-left coordinate.
   * - w, h: rectangle dimensions.
   * - color: 16-bit RGB565 color.
   * Outputs: Updates the canvas buffer.
   */
  void fillRect(int x, int y, int w, int h, uint16_t color) {
    for (int j = y; j < y + h; j++) {
      drawFastHLine(x, j, w, color);
    }
  }

  /**
   * Description: Draw a vertical line with clipping.
   * Inputs:
   * - x, y: start coordinate.
   * - h: line height.
   * - color: 16-bit RGB565 color.
   * Outputs: Updates the canvas buffer.
   */
  inline void drawFastVLine(int x, int y, int h, uint16_t color) {
    if ((x < 0) || (x >= _width) || (y >= _height)) return;
    if (y < 0) { h += y; y = 0; }
    if (y + h > _height) { h = _height - y; }
    uint16_t* p = _buffer + x + y * _stride;
    while (h-- > 0) {
      (*p) = color;
      p += _stride;
    }
  }

  /**
   * Description: Draw a horizontal line with clipping.
   * Inputs:
   * - x, y: start coordinate.
   * - w: line width.
   * - color: 16-bit RGB565 color.
   * Outputs: Updates the canvas buffer.
   */
  inline void drawFastHLine(int x, int y, int w, uint16_t color) {
    if ((y < 0) || (y >= _height) || (x >= _width)) return;
    if (x < 0) { w += x; x = 0; }
    if (x + w > _width) { w = _width - x; }
    uint16_t* p = _buffer + x + y * _stride;
    while (w-- > 0) {
      (*p) = color;
      p++;
    }
  }

 private:
  template <typename T>
  /**
   * Description: Swap two values (utility).
   * Inputs:
   * - a: first value.
   * - b: second value.
   * Outputs: Swaps the values in place.
   */
  inline static void swap(T& a, T& b) {
    T c = a;
    a = b;
    b = c;
  }

  uint16_t* _buffer = nullptr;
  int _width = 0;
  int _height = 0;
  int _stride = 0;
};
