#include <Arduino.h>
#include "Log.h"
#include "App.h"
#include "BoardPins.h"
#include "Faults.h"

/**
 * Description: Dispatch console commands for the application.
 * Inputs:
 * - msg: parsed command message.
 * Outputs: Executes command handling logic.
 */
static void handleConsoleCommand(const CommandMsg &msg) {
  LOGI("CONSOLE CMD: %s (args=%u)", msg.cmd, msg.argc);
  for (uint8_t i = 0; i < msg.argc; i++) {
    LOGI("  arg[%u]=%s", i, msg.argv[i]);
  }
}

/**
 * Description: Initialize application subsystems.
 * Inputs: None.
 * Outputs: Configures input, encoder, UI, show engine, and RS422 ports.
 */
void App::begin() {
  _console.setDispatchCommand(handleConsoleCommand);
  _console.begin();

  const bool buttonsOk = _buttons.begin();
  const bool ledsOk = _leds.begin();
  _analogs.begin();
  LOGI("Buttons begin: %s", buttonsOk ? "OK" : "FAIL");
  LOGI("Leds begin: %s", ledsOk ? "OK" : "FAIL");

  _enc.begin(PIN_ENC_A, PIN_ENC_B);

  _ui.begin();
  _show.begin();

  // Choose a starting baud for RoboClaw comms; we can change later.
  _rs422.begin(115200);

  _model.playing = false;
  _model.selectedMotor = 0;
}

/**
 * Description: Main application loop to update state and render UI.
 * Inputs: None.
 * Outputs: Updates model values and drives outputs each tick.
 */
void App::loop() {
  // Process console commands via USB serial of Teensy 4.1
  _console.poll();

  // Process button inputs from user interface.
  ButtonState buttonState = _buttons.poll();
  AnalogState analogState = _analogs.read();

  // Update the jog wheel encoder counts.
  const int32_t encoderDelta = _enc.consumeDelta();
  _model.jogPos += encoderDelta;

  // TEST CODE
  // Periodic serial test messages (10 Hz per port)
  static uint32_t lastSerialTick = 0;
  static uint32_t serialSeq[8] = {0};
  if (millis() - lastSerialTick >= 100) {
    lastSerialTick = millis();
    for (uint8_t i = 0; i < 8; i++) {
      auto* serialPort = _rs422.port(i).serial;
      if (serialPort) {
        serialPort->printf("Hello from port %u, %lu\r\n", (unsigned)i + 1, (unsigned long)serialSeq[i]++);
      }
    }

    // Dump status to console
    Serial.printf("%d, %d, %d\n", (int)(_model.speedNorm * 100.0f), (int)(_model.accelNorm * 100.0f), _model.jogPos);
  }

  // Dump serial port RX traffic.
  for (uint8_t i = 0; i < 8; i++) {
    auto* serialPort = _rs422.port(i).serial;
    if (serialPort) {
      static char line[64];
      line[0] = '\0';
      uint8_t count = 0;
      while (serialPort->available() && count < 16) {
        const int b = serialPort->read();
        const int written = snprintf(line + count * 3, sizeof(line) - count * 3, "%02X ", b & 0xFF);
        (void)written;
        count++;
      }
      if (count) {
        LOGI("SER %d: RX: %s", i, line);
      }

    }
  }
  
  // Toggle red led with red button
  if (buttonState.justPressed(Button::BUTTON_RED)) {
    if (_leds.getMode(LED::LED_RED_BUTTON) == LedMode::Off) {
      _leds.setMode(LED::LED_RED_BUTTON, LedMode::On);
      Serial.println("Red LED to ON");
    } else {
      _leds.setMode(LED::LED_RED_BUTTON, LedMode::Off);
      Serial.println("Red LED to OFF");
    }    
  }

  if (buttonState.justPressed(Button::BUTTON_YELLOW)) {
    if (_leds.getMode(LED::LED_YELLOW_BUTTON) == LedMode::Off) {
      _leds.setMode(LED::LED_YELLOW_BUTTON, LedMode::Blink);
      Serial.println("Yellow LED to Blink");
    } else {
      _leds.setMode(LED::LED_YELLOW_BUTTON, LedMode::Off);
      Serial.println("Yellow LED to Off");
    }    
  }

    if (buttonState.justPressed(Button::BUTTON_GREEN)) {
    if (_leds.getMode(LED::LED_GREEN_BUTTON) == LedMode::Off) {
      _leds.setMode(LED::LED_GREEN_BUTTON, LedMode::Blink, 50, 450);
      Serial.println("Green LED to Blink");
    } else {
      _leds.setMode(LED::LED_GREEN_BUTTON, LedMode::Off);
      Serial.println("Green LED to Off");
    }    
  }

  if (buttonState.justPressed(Button::BUTTON_OK)) {
    Serial.println("BUTTON: OK");
  }
  if (buttonState.justPressed(Button::BUTTON_DOWN))
  {
    Serial.println("BUTTON: DOWN");
  }
  if (buttonState.justPressed(Button::BUTTON_UP))
  {
    Serial.println("BUTTON: UP");
  }
  if (buttonState.justPressed(Button::BUTTON_LEFT))
  {
    Serial.println("BUTTON: LEFT");
  }
  if (buttonState.justPressed(Button::BUTTON_RIGHT))
  {
    Serial.println("BUTTON: RIGHT");
  }

  for (uint8_t i = 0; i < 8; i++) {
    if (buttonState.justPressed(static_cast<Button>(i))) {
      Serial.printf("BUTTON %d pressed!\n", i);
    }
  }
  
  // // Button test: log events and mirror button->LED
  //   const char* names[8] = {"BUTTON_LEFT","BUTTON_RIGHT","BUTTON_DOWN","BUTTON_UP","BUTTON_OK","BUTTON_RED","BUTTON_YELLOW","BUTTON_GREEN"};
  //   for (uint8_t i = 0; i < 8; i++) {
  //     if (in.isJustPressed[i]) {
  //       LOGI("BTN %s: pressed", names[i]);
  //       //_leds.setBlink(static_cast<LED>(i), 100, 100);
  //       _leds.setSteady(static_cast<LED>(i), 255); // full on (inverted in driver)
  //     } else if (in.isJustReleased[i]) {
  //       LOGI("BTN %s: released", names[i]);
  //       _leds.setSteady(static_cast<LED>(i), 0);   // off (inverted in driver)
  //     }
  //   }

  // // play toggle
  // if (buttonState.justPressed(Button::BUTTON_YELLOW)) {
  //   _model.playing = !_model.playing;
  //   _show.setPlaying(_model.playing);
  //   _leds.setMode(LED::LED_YELLOW_BUTTON, _model.playing ? LedMode::On : LedMode::Off);
  //   LOGI("BUTTON_YELLOW -> %s", _model.playing ? "ON" : "OFF");
  // }

  // // simple motor select with left/right for now
  // if (buttonState.justPressed(Button::BUTTON_UP)  && _model.selectedMotor > 0) _model.selectedMotor--;
  // if (buttonState.justPressed(Button::BUTTON_OK) && _model.selectedMotor < 15) _model.selectedMotor++; // up to 16 motors later

  // _model.showTimeMs = _show.currentTimeMs();
   _model.speedNorm  = analogState.potSpeedNorm;
   _model.accelNorm  = analogState.potAccelNorm;

  _leds.update();

  // Update the user interface outputs with latest status.
  _ui.render(_model);
}

App g_app;

/**
 * Description: Arduino setup entry point.
 * Inputs: None.
 * Outputs: Initializes logging and application subsystems.
 */
void setup() {
  logInit(115200);
  LOGI("Boot");

  g_app.begin();
}

/**
 * Description: Arduino loop entry point.
 * Inputs: None.
 * Outputs: Runs the application loop continuously.
 */
void loop() {
  g_app.loop();
}
