#include "Ui.h"
#include "BoardPins.h"
#include <SPI.h>
#include <Adafruit_GFX.h>
#include "DisplayConfig.h"

#if defined(DISPLAY_ILI9341_T4)
#include <ILI9341_T4.h>
#include "ILI9341Wrapper.h"
#else
#include "St7789T4Custom.h"
#endif

#if defined(DISPLAY_ILI9341_T4)
// Optimized ILI9341 driver (DMA, double-buffered)
static constexpr uint32_t ILI9341_SPI_HZ = 30000000;
static ILI9341Wrapper tft(PIN_LCD_CS, PIN_LCD_DC, 13, 11, 12, PIN_LCD_RST);
DMAMEM static uint16_t ili_internal_fb[320 * 240];
static ILI9341_T4::DiffBuffStatic<40000> diff1;
static ILI9341_T4::DiffBuffStatic<40000> diff2;
#else
// Optimized ST7789 driver for Teensy 4.x with configurable SPI speed
static St7789T4Custom tft(DISPLAY_RESOLUTION,
                          PIN_LCD_CS, PIN_LCD_DC, PIN_LCD_RST, PIN_LCD_BL,
                          ST7789_SPI_HZ_DEFAULT);
#endif

// Allocate canvas to match the rotated display dimensions at runtime.
static GFXcanvas16* canvas = nullptr;

static constexpr uint16_t COLOR_BLACK = 0x0000;
static constexpr uint16_t COLOR_WHITE = 0xFFFF;

static void flushCanvas() {
  if (!canvas) return;
#if defined(DISPLAY_ILI9341_T4)
  tft.update(canvas->getBuffer()); // DMA + diff buffers
#else
  tft.flush(0, 0, tft.width() - 1, tft.height() - 1, canvas->getBuffer());
#endif
}

void Ui::begin() {
  SPI.begin(); // init SPI0 before the driver starts transactions

#if defined(DISPLAY_ILI9341_T4)
  tft.output(nullptr);
  while (!tft.begin(ILI9341_SPI_HZ)) {}
  tft.setRotation(3); // 1 = landscape 320x240
  tft.setFramebuffer(ili_internal_fb);
  tft.setDiffBuffers(&diff1, &diff2);
  tft.setDiffGap(6);
  tft.setRefreshRate(60);
  tft.setVSyncSpacing(1);
#else
  tft.begin();
  tft.rotation(1); // landscape
#endif

  canvas = new GFXcanvas16(tft.width(), tft.height());
#if defined(DISPLAY_ILI9341_T4)
  tft.setCanvas(canvas->getBuffer(), tft.width(), tft.height());
#endif

  canvas->fillScreen(COLOR_BLACK);
  canvas->setTextWrap(false);
  canvas->setTextSize(2);
  canvas->setCursor(10, 10);
  canvas->setTextColor(COLOR_WHITE);
  canvas->print("Float Show Ctrl");
  flushCanvas();
}

void Ui::render(const UiModel& m) {

  if (_time_since_last_render < RENDER_PERIOD_MSEC)
    return;

  _time_since_last_render = 0;

  #ifdef TEST_GRID
  // Screen perimeter test pattern
  canvas->fillRect(0,0, tft.width(), tft.height(), ILI9341_T4_COLOR_NAVY);
  canvas->setCursor(0,0);
  canvas->setTextSize(2);
  canvas->printf("<---Size_2=12x16_chars--->\n");
  canvas->printf("01234567890123456789012345\n");
  canvas->printf("01234567890123456789012345\n");
  canvas->printf("01234567890123456789012345\n");
  canvas->printf("01234567890123456789012345\n");
  canvas->printf("01234567890123456789012345\n");
  canvas->printf("01234567890123456789012345\n");
  canvas->printf("01234567890123456789012345\n");
  canvas->printf("01234567890123456789012345\n");
  canvas->printf("01234567890123456789012345\n");
  canvas->printf("01234567890123456789012345\n");
  canvas->printf("01234567890123456789012345\n");
  canvas->printf("01234567890123456789012345\n");
  canvas->printf("01234567890123456789012345\n");
  canvas->printf("<---------BOTTOM--------->");
#endif

// Draw master screen background
canvas->fillRect(0,0, tft.width(), tft.height(), ILI9341_T4_COLOR_NAVY);

// Draw mode box in upper left corner
canvas->fillRect(0,0, 64, 32, ILI9341_T4_COLOR_RED);
canvas->setTextColor(ILI9341_T4_COLOR_WHITE);
canvas->setTextSize(2);
canvas->setCursor(6, 8); // Half character size from corner
canvas->printf("PLAY");

// Draw bottom button guide
canvas->fillRect(0,tft.height()-32, 106, tft.height(), ILI9341_T4_COLOR_RED);
canvas->fillRect(107,tft.height()-32, 214, tft.height(), ILI9341_T4_COLOR_YELLOW);
canvas->fillRect(215,tft.height()-32, tft.width(), tft.height(), ILI9341_T4_COLOR_GREEN);

// Red box guide
canvas->setCursor(8, tft.height()-30);
canvas->setTextColor(ILI9341_T4_COLOR_BLACK);
canvas->printf("RUN/HALT");
// Underline the active selection
canvas->drawFastHLine(8, tft.height()-10, 36, ILI9341_T4_COLOR_BLACK);

// Yellow box guide
canvas->setCursor(114, tft.height()-30);
canvas->setTextColor(ILI9341_T4_COLOR_BLACK);
canvas->printf("POS/SPD");
// Underline the active selection
canvas->drawFastHLine(114, tft.height()-10, 36, ILI9341_T4_COLOR_BLACK);

// Green box guide
canvas->setCursor(222, tft.height()-30);
canvas->setTextColor(ILI9341_T4_COLOR_BLACK);
canvas->printf("CHAN %d", m.selectedMotor);
// Underline the active selection
canvas->drawFastHLine(282, tft.height()-10, 16, ILI9341_T4_COLOR_BLACK);

// Main status area
canvas->setCursor(0, 48);
canvas->setTextColor(ILI9341_T4_COLOR_WHITE);
canvas->printf("SPEED: %3d%%\n", (int)(m.speedNorm * 100.0f));
canvas->printf("ACCEL: %3d%%\n", (int)(m.accelNorm * 100.0f));


  // simple overwrite HUD area
  // canvas->fillRect(0, 40, tft.width(), tft.height() - 80, COLOR_BLACK);
  // canvas->setCursor(10, 50);
  // canvas->setTextSize(2);

  // canvas->setTextColor(ILI9341_T4_COLOR_RED);
  // canvas->printf("Mode: %s\n", m.playing ? "PLAY" : "PAUSE");
  // canvas->setTextColor(ILI9341_T4_COLOR_GREEN);
  // canvas->printf("t: %lu ms\n", (unsigned long)m.showTimeMs);
  // canvas->printf("Motor: %u\n", m.selectedMotor);
  // canvas->printf("Speed: %d%%\n", (int)(m.speedNorm * 100.0f));
  // canvas->printf("Accel: %d%%\n", (int)(m.accelNorm * 100.0f));
  // canvas->printf("Jog: %ld\n", (long)m.jogPos);
  flushCanvas();
}
