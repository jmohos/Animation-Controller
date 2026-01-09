#include "Rs422Ports.h"

/**
 * Description: Select the HardwareSerial instance for a given port index.
 * Inputs:
 * - portIndex: index [0..7] for the RS422 port.
 * Outputs: Returns a pointer to the matching HardwareSerial instance.
 */
static HardwareSerial* pickSerialForIndex(uint8_t portIndex) {
  switch (portIndex) {
    case 0: return &Serial1;
    case 1: return &Serial2;
    case 2: return &Serial3;
    case 3: return &Serial4;
    case 4: return &Serial5;
    case 5: return &Serial6;
    case 6: return &Serial7;
    case 7: return &Serial8;
    default: return &Serial1;
  }
}

/**
 * Description: Initialize all RS422 ports to a given baud rate.
 * Inputs:
 * - baud: baud rate to use for every port.
 * Outputs: Initializes serial ports and stores handles.
 */
void Rs422Ports::begin(unsigned long baud) {
  for (uint8_t i = 0; i < 8; i++) {
    _ports[i].serial = pickSerialForIndex(i);
    _ports[i].serial->begin(baud);
  }
}
