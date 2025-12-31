#include <Arduino.h>
#include "Log.h"
#include "App.h"
#include "BoardPins.h"

void App::begin() {
  const bool ok = _input.begin();
  LOGI("Input begin: %s", ok ? "OK" : "FAIL");

  _enc.begin(PIN_ENC_A, PIN_ENC_B);

  _ui.begin();
  _show.begin();

  // Choose a starting baud for RoboClaw comms; we can change later.
  _rs422.begin(115200);

  _model.playing = false;
  _model.selectedMotor = 0;
}

void App::loop() {
  auto in = _input.poll();

  // encoder
  in.encoderDelta = _enc.consumeDelta();
  _model.jogPos += in.encoderDelta;

  // Periodic serial test messages (100 Hz per port)
  static uint32_t lastSerialTick = 0;
  static uint32_t serialSeq[8] = {0};
  if (millis() - lastSerialTick >= 10) {
    lastSerialTick = millis();
    for (uint8_t i = 0; i < 8; i++) {
      auto* s = _rs422.port(i).serial;
      if (s) {
        s->printf("Hello from port %u, %lu\r\n", (unsigned)i + 1, (unsigned long)serialSeq[i]++);
      }
    }
  }

  for (uint8_t i = 0; i < 8; i++) {
    auto* s = _rs422.port(i).serial;
    if (s) {
      static char line[64];
      line[0] = '\0';
      uint8_t count = 0;
      while (s->available() && count < 16) {
        const int b = s->read();
        const int written = snprintf(line + count * 3, sizeof(line) - count * 3, "%02X ", b & 0xFF);
        (void)written;
        count++;
      }
      if (count) {
        LOGI("SER %d: RX: %s", i, line);
      }

    }
  }
  
  if (in.justPressed(Button::Mode)) {
    if (_input.getLedMode(SXPin::LED_Red) == LedMode::Off) {
      _input.setLedMode(SXPin::LED_Red, LedMode::On);
      Serial.println("Red LED to ON");
    } else {
      _input.setLedMode(SXPin::LED_Red, LedMode::Off);
      Serial.println("Red LED to OFF");
    }    
  }

  if (in.justPressed(Button::Up)) {
    if (_input.getLedMode(SXPin::LED_Yellow) == LedMode::Off) {
      _input.setLedMode(SXPin::LED_Yellow, LedMode::Blink);
      Serial.println("Yellow LED to Blink");
    } else {
      _input.setLedMode(SXPin::LED_Yellow, LedMode::Off);
      Serial.println("Yellow LED to Off");
    }    
  }

    if (in.justPressed(Button::Down)) {
    if (_input.getLedMode(SXPin::LED_Green) == LedMode::Off) {
      _input.setLedMode(SXPin::LED_Green, LedMode::Blink, 200, 800);
      Serial.println("Green LED to Blink");
    } else {
      _input.setLedMode(SXPin::LED_Green, LedMode::Off);
      Serial.println("Green LED to Off");
    }    
  }

  // // Button test: log events and mirror button->LED
  //   const char* names[8] = {"Mode","Up","Down","Left","Right","OK","Play","Spare"};
  //   for (uint8_t i = 0; i < 8; i++) {
  //     if (in.isJustPressed[i]) {
  //       LOGI("BTN %s: pressed", names[i]);
  //       //_input.setLedBlink(i, 100, 100, 0, 255);
  //       _input.setLedSteady(i, 255); // full on (inverted in driver)
  //     } else if (in.isJustReleased[i]) {
  //       LOGI("BTN %s: released", names[i]);
  //       _input.setLedSteady(i, 0);   // off (inverted in driver)
  //     }
  //   }

  // // play toggle
  // if (in.justPressed(Button::Play)) {
  //   _model.playing = !_model.playing;
  //   _show.setPlaying(_model.playing);
  //   _input.setPlayLed(_model.playing);
  //   LOGI("Play -> %s", _model.playing ? "ON" : "OFF");
  // }

  // // simple motor select with left/right for now
  // if (in.justPressed(Button::Left)  && _model.selectedMotor > 0) _model.selectedMotor--;
  // if (in.justPressed(Button::Right) && _model.selectedMotor < 15) _model.selectedMotor++; // up to 16 motors later

  // _model.showTimeMs = _show.currentTimeMs();
   _model.speedNorm  = in.potSpeedNorm;
   _model.accelNorm  = in.potAccelNorm;

  _ui.render(_model);
}

App g_app;

void setup() {
  logInit(115200);
  LOGI("Boot");

  g_app.begin();
}

void loop() {
  g_app.loop();
}
