#pragma once
#include <Arduino.h>
#include "BoardPins.h"

struct Rs422Port {
  HardwareSerial* serial = nullptr;
};

class Rs422Ports {
public:
  void begin(unsigned long baud);
  Rs422Port& port(uint8_t idx) { return _ports[idx]; }

private:
  Rs422Port _ports[8];
};
