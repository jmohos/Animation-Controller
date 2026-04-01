#include "Ui.h"
#include "BoardPins.h"
#include "Faults.h"
#include "ILI9341Wrapper.h"
#include "StaticConfig.h"
#include <Adafruit_GFX.h>
#include <ILI9341_T4.h>
#include <SPI.h>

static constexpr uint32_t ILI9341_SPI_HZ = 30000000;
static ILI9341Wrapper tft(PIN_LCD_CS, PIN_LCD_DC, 13, 11, 255, PIN_LCD_RST);
DMAMEM static uint16_t ili_internal_fb[320 * 240];
static ILI9341_T4::DiffBuffStatic<40000> diff1;
static ILI9341_T4::DiffBuffStatic<40000> diff2;
static GFXcanvas16* canvas = nullptr;

namespace {
const char *modeLabel(const UiModel &model) {
  switch (model.mode) {
    case ControllerMode::Stop: return "STOPPED";
    case ControllerMode::Run: return "RUNNING";
    case ControllerMode::ManualHold: return "MANUAL HOLD";
    case ControllerMode::ManualRun: return "MANUAL RUN";
    default: return "UNKNOWN";
  }
}

const char *manualButtonLabel(ControllerMode mode) {
  return (mode == ControllerMode::ManualHold) ? "MAN RUN" : "MAN HLD";
}

uint16_t centeredTextX(uint16_t boxX, uint16_t boxWidth, const char *text, uint8_t textSize) {
  const uint16_t charWidth = 6 * textSize;
  const uint16_t textWidth = strlen(text) * charWidth;
  return boxX + ((boxWidth - textWidth) / 2);
}

const char *parameterUnit(ManualParameter parameter) {
  switch (parameter) {
    case ManualParameter::SpeedPercent: return "%";
    case ManualParameter::VelocityDegPerSec: return "deg/sec";
    case ManualParameter::PositionDeg: return "deg";
    case ManualParameter::AudioTrigger: return "trigger";
    default: return "";
  }
}

uint16_t modeColor(ControllerMode mode) {
  switch (mode) {
    case ControllerMode::Stop: return ILI9341_T4_COLOR_RED;
    case ControllerMode::Run: return ILI9341_T4_COLOR_GREEN;
    case ControllerMode::ManualHold:
    case ControllerMode::ManualRun:
      return ILI9341_T4_COLOR_YELLOW;
    default:
      return ILI9341_T4_COLOR_WHITE;
  }
}

void flushCanvas() {
  if (canvas) {
    tft.update(canvas->getBuffer());
  }
}
}

void Ui::begin() {
  _ready = false;
  SPI.begin();

  tft.output(nullptr);
  if (!tft.begin(ILI9341_SPI_HZ)) {
    FAULT_SET(FAULT_LCD_DISPLAY_FAULT);
    return;
  }
  tft.invertDisplay(true);
  tft.setRotation(1);
  tft.setFramebuffer(ili_internal_fb);
  tft.setDiffBuffers(&diff1, &diff2);
  tft.setDiffGap(6);
  tft.setRefreshRate(60);
  tft.setVSyncSpacing(1);

  canvas = new GFXcanvas16(tft.width(), tft.height());
  if (!canvas) {
    FAULT_SET(FAULT_LCD_DISPLAY_FAULT);
    return;
  }

  tft.setCanvas(canvas->getBuffer(), tft.width(), tft.height());
  canvas->fillScreen(ILI9341_T4_COLOR_BLACK);
  canvas->setTextWrap(false);
  canvas->setTextColor(ILI9341_T4_COLOR_WHITE);
  canvas->setTextSize(2);
  canvas->setCursor(12, 12);
  canvas->print("Animation Ctrl");
  flushCanvas();
  _ready = true;
}

void Ui::render(const UiModel& model) {
  if (!_ready) {
    return;
  }
  if (_timeSinceLastRender < RENDER_PERIOD_MSEC) {
    return;
  }
  _timeSinceLastRender = 0;

  canvas->fillScreen(ILI9341_T4_COLOR_NAVY);
  canvas->setTextColor(ILI9341_T4_COLOR_WHITE);

  canvas->fillRect(0, 0, 160, 36, modeColor(model.mode));
  canvas->setTextSize(2);
  if (model.mode == ControllerMode::Run) {
    char modeText[20] = {};
    snprintf(modeText, sizeof(modeText), "%s %u", modeLabel(model), (unsigned)model.runPattern);
    canvas->setTextColor(ILI9341_T4_COLOR_BLACK);
    canvas->setCursor(centeredTextX(0, 160, modeText, 2), 10);
    canvas->print(modeText);
  } else {
    const char *label = modeLabel(model);
    canvas->setTextColor((model.mode == ControllerMode::ManualHold || model.mode == ControllerMode::ManualRun)
                         ? ILI9341_T4_COLOR_BLACK
                         : ILI9341_T4_COLOR_WHITE);
    canvas->setCursor(centeredTextX(0, 160, label, 2), 10);
    canvas->print(label);
  }
  canvas->setTextColor(ILI9341_T4_COLOR_WHITE);

  canvas->fillRect(0, 208, 106, 32, ILI9341_T4_COLOR_RED);
  canvas->fillRect(107, 208, 106, 32, ILI9341_T4_COLOR_YELLOW);
  canvas->fillRect(214, 208, 106, 32, ILI9341_T4_COLOR_GREEN);
  canvas->setTextColor(ILI9341_T4_COLOR_BLACK);
  canvas->setCursor(centeredTextX(0, 106, "STOP", 2), 216);
  canvas->print("STOP");
  canvas->setCursor(centeredTextX(107, 106, manualButtonLabel(model.mode), 2), 216);
  canvas->print(manualButtonLabel(model.mode));
  canvas->setCursor(centeredTextX(214, 106, "RUN", 2), 216);
  canvas->print("RUN");
  canvas->setTextColor(ILI9341_T4_COLOR_WHITE);

  if (model.bootScreen) {
    canvas->setCursor(12, 64);
    canvas->setTextSize(2);
    canvas->print("BOOTING...");
    flushCanvas();
    return;
  }

  if (model.mode == ControllerMode::Run) {
    canvas->setTextSize(2);
    canvas->setCursor(36, 74);
    canvas->printf("SPEED: %u%%", (unsigned)model.runSpeedScale);

    const bool blinkOn = ((millis() / 250u) & 1u) == 0u;
    canvas->setCursor(36, 114);
    if (!model.runPatternPending || blinkOn) {
      canvas->printf("ANIM #: %u", (unsigned)model.pendingRunPattern);
    }

    canvas->setTextSize(1);
    canvas->setCursor(36, 156);
    canvas->print("KNOB=SPEED  UP/DN=PATTERN  OK=APPLY");
  } else if (model.mode == ControllerMode::ManualHold) {
    canvas->setTextSize(1);
    const uint8_t rowHeight = 18;
    const uint16_t startY = 46;
    const uint16_t cellWidth = 142;
    const uint16_t cellHeight = 12;
    for (uint8_t i = 0; i < kStaticItemCount; i++) {
      const uint8_t row = i / kManualGridColumns;
      const uint8_t col = i % kManualGridColumns;
      const uint16_t x = (col == 0) ? 12 : 166;
      const uint16_t y = startY + row * rowHeight;
      if (i == model.selectedManualItem) {
        canvas->fillRect(x - 2, y - 1, cellWidth, cellHeight, ILI9341_T4_COLOR_WHITE);
        canvas->setTextColor(ILI9341_T4_COLOR_BLACK);
      } else {
        canvas->setTextColor(ILI9341_T4_COLOR_WHITE);
      }
      canvas->setCursor(x, y);
      canvas->print(kStaticItems[i].name);
    }
    canvas->setTextColor(ILI9341_T4_COLOR_WHITE);
    canvas->setCursor(12, 186);
    canvas->print("UDLR=SELECT  YELLOW=RUN");
  } else if (model.mode == ControllerMode::ManualRun) {
    const StaticItemDef &item = kStaticItems[model.selectedManualItem];
    canvas->setTextSize(2);
    canvas->setCursor(12, 70);
    canvas->print(item.name);
    canvas->setCursor(12, 110);
    if (item.parameter == ManualParameter::AudioTrigger) {
      canvas->printf("TRACK: %ld", (long)model.manualValue);
    } else {
      canvas->printf("%ld %s", (long)model.manualValue, parameterUnit(model.manualParameter));
    }
    canvas->setTextSize(1);
    canvas->setCursor(12, 150);
    canvas->printf("RANGE %ld..%ld", (long)model.manualMin, (long)model.manualMax);
    canvas->setCursor(12, 164);
    canvas->print("KNOB=ADJUST  YELLOW=HOLD");
    if (item.parameter == ManualParameter::AudioTrigger) {
      canvas->setCursor(12, 178);
      canvas->print("OK=TRIGGER");
    }
  }

  flushCanvas();
}
