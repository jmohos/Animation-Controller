#pragma once
#include <Arduino.h>
#include "BoardPins.h"

struct Rs422Port {
  HardwareSerial* serial = nullptr;
};

class Rs422Ports {
public:
  /**
   * Description: Initialize all RS422 serial ports at a common baud rate.
   * Inputs:
   * - baud: baud rate to apply to all ports.
   * Outputs: Opens serial ports and stores handles.
   */
  void begin(unsigned long baud);

  /**
   * Description: Access a specific RS422 port by index.
   * Inputs:
   * - portIndex: port index [0..7].
   * Outputs: Returns a reference to the port entry.
   */
  Rs422Port& port(uint8_t portIndex) { return _ports[portIndex]; }

private:
  Rs422Port _ports[8];
};
