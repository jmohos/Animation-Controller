#include "Ui.h"
#include "BoardPins.h"
#include "Faults.h"
#include "MenuDefs.h"
#include <SPI.h>
#include <Adafruit_GFX.h>

#include <ILI9341_T4.h>
#include "ILI9341Wrapper.h"

// Optimized ILI9341 driver (DMA, double-buffered)
static constexpr uint32_t ILI9341_SPI_HZ = 30000000;
// MISO disabled to allow MOSI-only display wiring.
static ILI9341Wrapper tft(PIN_LCD_CS, PIN_LCD_DC, 13, 11, 255, PIN_LCD_RST);
DMAMEM static uint16_t ili_internal_fb[320 * 240];
static ILI9341_T4::DiffBuffStatic<40000> diff1;
static ILI9341_T4::DiffBuffStatic<40000> diff2;

// Allocate canvas to match the rotated display dimensions at runtime.
static GFXcanvas16* canvas = nullptr;

static constexpr uint16_t COLOR_BLACK = 0x0000;
static constexpr uint16_t COLOR_WHITE = 0xFFFF;

static const char* screenLabel(UiModel::Screen screen) {
  switch (screen) {
    case UiModel::Screen::Boot: return "BOOT";
    case UiModel::Screen::Manual: return "MAN";
    case UiModel::Screen::Auto: return "AUTO";
    case UiModel::Screen::Edit: return "EDIT";
    case UiModel::Screen::Menu: return "MENU";
    case UiModel::Screen::Settings: return "SET";
    case UiModel::Screen::Diagnostics: return "DIAG";
    case UiModel::Screen::Endpoints: return "EP";
    case UiModel::Screen::EndpointConfig: return "EPC";
    case UiModel::Screen::EndpointConfigEdit: return "EPD";
    case UiModel::Screen::RoboClawStatus: return "RC";
    default: return "UNK";
  }
}

static void drawMenuList(const char *title, const MenuItem *items, uint8_t count, uint8_t selected) {
  canvas->printf("%s\n", title);
  for (uint8_t i = 0; i < count; i++) {
    canvas->setCursor(10, 72 + (i * 20));
    canvas->printf("%c %s", (i == selected) ? '>' : ' ', items[i].label);
  }
}

static void drawEndpointList(const UiModel &model) {
  canvas->printf("ENDPOINTS\n");
  canvas->setTextSize(1);
  const uint16_t startY = 80;
  const uint8_t lineHeight = 10;
  for (uint8_t i = 0; i < MAX_ENDPOINTS; i++) {
    const uint16_t y = startY + static_cast<uint16_t>(i * lineHeight);
    canvas->setCursor(0, y);
    const char cursor = (i == model.selectedMotor) ? '>' : ' ';
    const uint8_t epNum = static_cast<uint8_t>(i + 1);
    if (!model.endpointEnabled[i]) {
      canvas->printf("%c%02u DISABLED", cursor, epNum);
      continue;
    }
    if (!model.endpointStatusValid[i]) {
      canvas->printf("%c%02u NO DATA", cursor, epNum);
      continue;
    }
    canvas->printf("%c%02u ", cursor, epNum);
    if (model.endpointEncValid[i]) {
      canvas->printf("P:%ld ", (long)model.endpointPos[i]);
    } else {
      canvas->printf("P:-- ");
    }
    if (model.endpointSpeedValid[i]) {
      canvas->printf("S:%ld ", (long)model.endpointSpeed[i]);
    } else {
      canvas->printf("S:-- ");
    }
  }
  canvas->setTextSize(2);
}

static void drawEndpointConfigList(const UiModel &model) {
  canvas->printf("ENDPOINT CFG\n");
  canvas->setTextSize(1);
  const uint16_t startY = 80;
  const uint8_t lineHeight = 10;
  for (uint8_t i = 0; i < MAX_ENDPOINTS; i++) {
    const uint16_t y = startY + static_cast<uint16_t>(i * lineHeight);
    canvas->setCursor(0, y);
    const char cursor = (i == model.endpointConfigIndex) ? '>' : ' ';
    const uint8_t epNum = static_cast<uint8_t>(i + 1);
    const bool enabled = model.endpointEnabled[i];
    canvas->printf("%c%02u %s %s A%08lX",
                   cursor,
                   epNum,
                   enabled ? "EN " : "DIS",
                   endpointTypeShortName(model.endpointConfigType[i]),
                   (unsigned long)model.endpointConfigAddress[i]);
    if (model.endpointConfigType[i] == EndpointType::RoboClaw) {
      canvas->printf(" S%u M%u",
                     (unsigned)model.endpointConfigPort[i],
                     (unsigned)model.endpointConfigMotor[i]);
    }
  }
  canvas->setTextSize(2);
}

static void drawEndpointConfigEdit(const UiModel &model) {
  const EndpointConfig &ep = model.endpointConfigSelected;
  canvas->printf("EP%02u %s\n",
                 (unsigned)(model.endpointConfigIndex + 1),
                 model.endpointConfigEditing ? "EDIT" : "CFG");

  const uint16_t startY = 72;
  const uint8_t lineHeight = 16;
  const uint8_t fieldsPerPage = 6;
  const uint8_t fieldStart = static_cast<uint8_t>((model.endpointConfigField / fieldsPerPage) * fieldsPerPage);

  for (uint8_t i = 0; i < fieldsPerPage; i++) {
    const uint8_t field = static_cast<uint8_t>(fieldStart + i);
    if (field >= ENDPOINT_FIELD_COUNT) {
      break;
    }
    const uint16_t y = startY + static_cast<uint16_t>(i * lineHeight);
    canvas->setCursor(0, y);
    const char cursor = (model.endpointConfigField == field) ? '>' : ' ';
    switch (static_cast<EndpointField>(field)) {
      case EndpointField::Enabled:
        canvas->printf("%c ENABLE: %s", cursor, ep.enabled ? "ON" : "OFF");
        break;
      case EndpointField::Type:
        canvas->printf("%c TYPE:   %s", cursor, endpointTypeName(ep.type));
        break;
      case EndpointField::Address:
        canvas->printf("%c ADDR:   0x%08lX", cursor, (unsigned long)ep.address);
        break;
      case EndpointField::SerialPort:
        canvas->printf("%c SERIAL: %u", cursor, (unsigned)ep.serialPort);
        break;
      case EndpointField::Motor:
        canvas->printf("%c MOTOR:  %u", cursor, (unsigned)ep.motor);
        break;
      case EndpointField::PositionMin:
        canvas->printf("%c PMIN:   %ld", cursor, (long)ep.positionMin);
        break;
      case EndpointField::PositionMax:
        canvas->printf("%c PMAX:   %ld", cursor, (long)ep.positionMax);
        break;
      case EndpointField::VelocityMin:
        canvas->printf("%c VMIN:   %lu", cursor, (unsigned long)ep.velocityMin);
        break;
      case EndpointField::VelocityMax:
        canvas->printf("%c VMAX:   %lu", cursor, (unsigned long)ep.velocityMax);
        break;
      case EndpointField::AccelMin:
        canvas->printf("%c AMIN:   %lu", cursor, (unsigned long)ep.accelMin);
        break;
      case EndpointField::AccelMax:
        canvas->printf("%c AMAX:   %lu", cursor, (unsigned long)ep.accelMax);
        break;
      default:
        break;
    }
  }

  canvas->setTextSize(1);
  canvas->setCursor(0, 210);
  if (model.endpointConfigEditing) {
    canvas->printf("UP/DN=ADJ OK=DONE LT=BACK");
  } else {
    canvas->printf("UP/DN=FIELD OK=EDIT LT=BACK");
  }
  canvas->setTextSize(2);
}

static void drawRoboClawStatusList(const UiModel &model) {
  canvas->printf("ROBOCLAW\n");
  canvas->setTextSize(1);
  const uint16_t startY = 80;
  const uint8_t lineHeight = 10;
  uint8_t lines = 0;
  for (uint8_t i = 0; i < MAX_RC_PORTS; i++) {
    if (!model.rcPortEnabled[i]) {
      continue;
    }
    const uint16_t y = startY + static_cast<uint16_t>(lines * lineHeight);
    canvas->setCursor(0, y);
    canvas->printf("S%u A%02X ",
                   (unsigned)(i + 1),
                   (unsigned)model.rcPortAddress[i]);
    if (!model.rcPortStatusValid[i]) {
      canvas->printf("NO DATA");
      lines++;
      continue;
    }
    if (model.rcPortEncValid[i]) {
      canvas->printf("P:%ld/%ld ", (long)model.rcPortEnc1[i], (long)model.rcPortEnc2[i]);
    } else {
      canvas->printf("P:--/-- ");
    }
    if (model.rcPortSpeedValid[i]) {
      canvas->printf("S:%ld/%ld ", (long)model.rcPortSpeed1[i], (long)model.rcPortSpeed2[i]);
    } else {
      canvas->printf("S:--/-- ");
    }
    if (model.rcPortErrorValid[i]) {
      canvas->printf("E:%08lX", (unsigned long)model.rcPortError[i]);
    } else {
      canvas->printf("E:--");
    }
    lines++;
  }
  if (lines == 0) {
    canvas->setCursor(0, startY);
    canvas->printf("NO ENABLED ROBOCLAWS");
  }
  canvas->setTextSize(2);
}

static void drawSequenceProgressBar(const UiModel &model) {
  if (!model.sequenceLoaded || model.sequenceLoopMs == 0) {
    return;
  }
  const uint16_t barX = 10;
  const uint16_t barY = 190;
  const uint16_t barW = 300;
  const uint16_t barH = 10;
  uint32_t progress = model.showTimeMs;
  if (progress > model.sequenceLoopMs) {
    progress = model.sequenceLoopMs;
  }
  const uint16_t fillW = static_cast<uint16_t>(
    (static_cast<uint64_t>(progress) * barW) / model.sequenceLoopMs
  );

  canvas->drawRect(barX, barY, barW, barH, ILI9341_T4_COLOR_WHITE);
  if (fillW > 0) {
    canvas->fillRect(barX + 1, barY + 1, fillW - 1, barH - 2, ILI9341_T4_COLOR_GREEN);
  }
}

static void drawEditScreen(const UiModel &model) {
  canvas->printf("EDIT EP %u\n", (unsigned)(model.selectedMotor + 1));
  if (!model.editHasEvent) {
    canvas->printf("NO EVENTS\n");
    canvas->setTextSize(1);
    canvas->setCursor(0, 120);
    canvas->printf("YEL=ADD  GRN=EP");
    canvas->setTextSize(2);
    return;
  }

  canvas->printf("STEP %u/%u\n",
                 (unsigned)model.editEventOrdinal,
                 (unsigned)model.editEventCount);
  canvas->setTextSize(1);
  canvas->setCursor(0, 96);
  canvas->printf("%c TIME: %lu ms\n",
                 (model.editField == 0) ? '>' : ' ',
                 (unsigned long)model.editTimeMs);
  canvas->printf("%c POS:  %ld\n",
                 (model.editField == 1) ? '>' : ' ',
                 (long)model.editPosition);
  canvas->printf("%c VEL:  %lu\n",
                 (model.editField == 2) ? '>' : ' ',
                 (unsigned long)model.editVelocity);
  canvas->printf("%c ACC:  %lu\n",
                 (model.editField == 3) ? '>' : ' ',
                 (unsigned long)model.editAccel);
  canvas->setCursor(0, 200);
  canvas->printf("UP/DN=STEP OK=FIELD ENC=ADJ");
  canvas->setTextSize(2);
}

/**
 * Description: Flush the current canvas buffer to the display.
 * Inputs: None.
 * Outputs: Sends the canvas buffer to the display driver.
 */
static void flushCanvas() {
  if (!canvas) return;
  tft.update(canvas->getBuffer()); // DMA + diff buffers
}

/**
 * Description: Initialize the display driver and draw the startup screen.
 * Inputs: None.
 * Outputs: Configures the display and initializes the canvas buffer.
 */
void Ui::begin() {
  _ready = false;
  SPI.begin(); // init SPI0 before the driver starts transactions

  tft.output(nullptr);
  if (!tft.begin(ILI9341_SPI_HZ)) {
    FAULT_SET(FAULT_LCD_DISPLAY_FAULT);
    return;
  }
  tft.invertDisplay(true);
  tft.setRotation(1); // 1 = landscape 320x240
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

  canvas->fillScreen(COLOR_BLACK);
  canvas->setTextWrap(false);
  canvas->setTextSize(2);
  canvas->setCursor(10, 10);
  canvas->setTextColor(COLOR_WHITE);
  canvas->print("Float Show Ctrl");
  flushCanvas();
  _ready = true;
}

/**
 * Description: Render the main UI at a fixed cadence.
 * Inputs:
 * - model: UI model data to draw.
 * Outputs: Updates the canvas and flushes to the display.
 */
void Ui::render(const UiModel& model) {
  if (!_ready) {
    return;
  }

  if (_timeSinceLastRender < RENDER_PERIOD_MSEC)
    return;

  _timeSinceLastRender = 0;

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
  canvas->printf("%s", screenLabel(model.screen));

  const bool showHotkeys = (model.screen == UiModel::Screen::Manual ||
                            model.screen == UiModel::Screen::Auto ||
                            model.screen == UiModel::Screen::Edit);
  if (showHotkeys) {
    // Draw bottom button guide
    canvas->fillRect(0,tft.height()-32, 106, tft.height(), ILI9341_T4_COLOR_RED);
    canvas->fillRect(107,tft.height()-32, 214, tft.height(), ILI9341_T4_COLOR_YELLOW);
    canvas->fillRect(215,tft.height()-32, tft.width(), tft.height(), ILI9341_T4_COLOR_GREEN);
  }

  const char* hintRed = "N/A";
  const char* hintYellow = "N/A";
  char hintGreen[16] = "N/A";
  switch (model.screen) {
    case UiModel::Screen::Manual:
      hintRed = model.playing ? "HALT" : "RUN";
      hintYellow = "MODE";
      snprintf(hintGreen, sizeof(hintGreen), "EP %u", (unsigned)(model.selectedMotor + 1));
      break;
    case UiModel::Screen::Auto:
      hintRed = model.playing ? "PAUSE" : "PLAY";
      hintYellow = "MODE";
      snprintf(hintGreen, sizeof(hintGreen), "EP %u", (unsigned)(model.selectedMotor + 1));
      break;
    case UiModel::Screen::Edit:
      hintRed = "DEL";
      hintYellow = "ADD";
      snprintf(hintGreen, sizeof(hintGreen), "EP %u", (unsigned)(model.selectedMotor + 1));
      break;
    default:
      break;
  }

  if (showHotkeys) {
    // Red box guide
    canvas->setCursor(8, tft.height()-30);
    canvas->setTextColor(ILI9341_T4_COLOR_BLACK);
    canvas->printf("%s", hintRed);
    canvas->drawFastHLine(8, tft.height()-10, 36, ILI9341_T4_COLOR_BLACK);

    // Yellow box guide
    canvas->setCursor(114, tft.height()-30);
    canvas->setTextColor(ILI9341_T4_COLOR_BLACK);
    canvas->printf("%s", hintYellow);
    canvas->drawFastHLine(114, tft.height()-10, 36, ILI9341_T4_COLOR_BLACK);

    // Green box guide
    canvas->setCursor(222, tft.height()-30);
    canvas->setTextColor(ILI9341_T4_COLOR_BLACK);
    canvas->printf("%s", hintGreen);
    canvas->drawFastHLine(282, tft.height()-10, 16, ILI9341_T4_COLOR_BLACK);
  }

  // Main status area
  canvas->setCursor(0, 48);
  canvas->setTextColor(ILI9341_T4_COLOR_WHITE);
  canvas->setTextSize(2);
  switch (model.screen) {
    case UiModel::Screen::Boot:
      canvas->printf("BOOTING...\n");
      if (model.statusLine[0] != '\0') {
        canvas->printf("%s\n", model.statusLine);
      }
      break;
    case UiModel::Screen::Manual:
      canvas->printf("SPEED: %3d%%\n", (int)(model.speedNorm * 100.0f));
      canvas->printf("ACCEL: %3d%%\n", (int)(model.accelNorm * 100.0f));
      canvas->printf("JOG: %ld\n", (long)model.jogPos);
      if (model.rcEncValid) {
        canvas->printf("POS: %ld\n", (long)model.rcSelectedEnc);
      } else {
        canvas->printf("POS: --\n");
      }
      if (model.rcSpeedValid) {
        canvas->printf("SPD: %ld\n", (long)model.rcSelectedSpeed);
      } else {
        canvas->printf("SPD: --\n");
      }
      if (model.rcErrorValid) {
        canvas->printf("ERR: 0x%08lX\n", (unsigned long)model.rcError);
      }
      break;
    case UiModel::Screen::Auto:
      canvas->printf("TIME: %lu ms\n", (unsigned long)model.showTimeMs);
      canvas->printf("STATE: %s\n", model.playing ? "PLAY" : "PAUSE");
      if (model.rcEncValid) {
        canvas->printf("POS: %ld\n", (long)model.rcSelectedEnc);
      } else {
        canvas->printf("POS: --\n");
      }
      if (model.rcSpeedValid) {
        canvas->printf("SPD: %ld\n", (long)model.rcSelectedSpeed);
      } else {
        canvas->printf("SPD: --\n");
      }
      if (model.rcErrorValid) {
        canvas->printf("ERR: 0x%08lX\n", (unsigned long)model.rcError);
      }
      canvas->printf("SEQ: %s (%u)\n", model.sequenceLoaded ? "OK" : "NONE", (unsigned)model.sequenceCount);
      if (model.sequenceLoaded) {
        canvas->printf("LOOP: %lu ms\n", (unsigned long)model.sequenceLoopMs);
      }
      drawSequenceProgressBar(model);
      break;
    case UiModel::Screen::Menu: {
      drawMenuList("MENU", MENU_ITEMS, MENU_ITEM_COUNT, model.menuIndex);
      break;
    }
    case UiModel::Screen::Endpoints:
      canvas->setTextSize(1);
      canvas->setCursor(0, 66);
      canvas->printf("UP/DN=SELECT  LEFT=BACK");
      canvas->setTextSize(2);
      canvas->setCursor(0, 48);
      drawEndpointList(model);
      break;
    case UiModel::Screen::EndpointConfig:
      canvas->setCursor(0, 48);
      drawEndpointConfigList(model);
      break;
    case UiModel::Screen::EndpointConfigEdit:
      canvas->setCursor(0, 48);
      drawEndpointConfigEdit(model);
      break;
    case UiModel::Screen::RoboClawStatus:
      canvas->setCursor(0, 48);
      drawRoboClawStatusList(model);
      break;
    case UiModel::Screen::Settings:
      drawMenuList("SETTINGS", SETTINGS_ITEMS, SETTINGS_ITEM_COUNT, model.settingsIndex);
      canvas->setCursor(0, 120);
      canvas->printf("UP/DN=SELECT\nOK=RUN\nLEFT=BACK\n");
      canvas->printf("SD: %s\n", model.sdReady ? "OK" : "ERR");
      if (model.statusLine[0] != '\0') {
        canvas->printf("%s\n", model.statusLine);
      }
      break;
    case UiModel::Screen::Diagnostics:
      drawMenuList("DIAGNOSTICS", DIAGNOSTICS_ITEMS, DIAGNOSTICS_ITEM_COUNT, model.diagnosticsIndex);
      canvas->setCursor(0, 148);
      canvas->printf("SD: %s\n", model.sdReady ? "OK" : "ERR");
      if (model.statusLine[0] != '\0') {
        canvas->printf("%s\n", model.statusLine);
      }
      break;
    case UiModel::Screen::Edit:
      canvas->setCursor(0, 48);
      drawEditScreen(model);
      break;
    default:
      break;
  }

  if (model.screen == UiModel::Screen::Diagnostics) {
    auto stats = tft.statsFPS();
    canvas->setCursor(0, 210);
    canvas->printf("FPS: %0.1f\n", stats.avg());
  }

// simple overwrite HUD area
// canvas->fillRect(0, 40, tft.width(), tft.height() - 80, COLOR_BLACK);
// canvas->setCursor(10, 50);
// canvas->setTextSize(2);

// canvas->setTextColor(ILI9341_T4_COLOR_RED);
// canvas->printf("BUTTON_LEFT: %s\n", model.playing ? "PLAY" : "PAUSE");
// canvas->setTextColor(ILI9341_T4_COLOR_GREEN);
// canvas->printf("t: %lu ms\n", (unsigned long)model.showTimeMs);
// canvas->printf("Motor: %u\n", model.selectedMotor);
// canvas->printf("Speed: %d%%\n", (int)(model.speedNorm * 100.0f));
// canvas->printf("Accel: %d%%\n", (int)(model.accelNorm * 100.0f));
// canvas->printf("Jog: %ld\n", (long)model.jogPos);
flushCanvas();
}
