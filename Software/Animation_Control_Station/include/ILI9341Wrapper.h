#pragma once

#include <Arduino.h>
#include <ILI9341_T4.h>

// Minimal wrapper adding canvas helpers used by the demosauce example.
class ILI9341Wrapper : public ILI9341_T4::ILI9341Driver {
 public:
  ILI9341Wrapper(uint8_t cs, uint8_t dc, uint8_t sclk, uint8_t mosi, uint8_t miso,
                 uint8_t rst = 255, uint8_t touch_cs = 255, uint8_t touch_irq = 255)
      : ILI9341Driver(cs, dc, sclk, mosi, miso, rst, touch_cs, touch_irq) {}

  void setCanvas(uint16_t* fb, int lx, int ly) {
    _buffer = fb;
    _lx = lx;
    _ly = ly;
    _stride = lx;
  }

  inline void drawPixel(int x, int y, uint16_t color) {
    if ((x < 0) || (y < 0) || (x >= _lx) || (y >= _ly)) return;
    _buffer[x + _stride * y] = color;
  }

  inline uint16_t readPixel(int x, int y) {
    if ((x < 0) || (y < 0) || (x >= _lx) || (y >= _ly)) return 0;
    return _buffer[x + _stride * y];
  }

  void fillScreen(uint16_t color) { fillRect(0, 0, _lx, _ly, color); }

  void fillRect(int x, int y, int w, int h, uint16_t color) {
    for (int j = y; j < y + h; j++) {
      drawFastHLine(x, j, w, color);
    }
  }

  inline void drawFastVLine(int x, int y, int h, uint16_t color) {
    if ((x < 0) || (x >= _lx) || (y >= _ly)) return;
    if (y < 0) { h += y; y = 0; }
    if (y + h > _ly) { h = _ly - y; }
    uint16_t* p = _buffer + x + y * _stride;
    while (h-- > 0) {
      (*p) = color;
      p += _stride;
    }
  }

  inline void drawFastHLine(int x, int y, int w, uint16_t color) {
    if ((y < 0) || (y >= _ly) || (x >= _lx)) return;
    if (x < 0) { w += x; x = 0; }
    if (x + w > _lx) { w = _lx - x; }
    uint16_t* p = _buffer + x + y * _stride;
    while (w-- > 0) {
      (*p) = color;
      p++;
    }
  }

 private:
  template <typename T>
  inline static void swap(T& a, T& b) {
    T c = a;
    a = b;
    b = c;
  }

  uint16_t* _buffer = nullptr;
  int _lx = 0;
  int _ly = 0;
  int _stride = 0;
};
