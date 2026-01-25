#include "Rs422Ports.h"

/**
 * Description: Select the HardwareSerial instance for a given port index.
 * Inputs:
 * - portIndex: index [0..6] for the RS422 port.
 * Outputs: Returns a pointer to the matching HardwareSerial instance.
 */
static HardwareSerial* pickSerialForIndex(uint8_t portIndex) {
  switch (portIndex) {
    case 0: return &Serial2;
    case 1: return &Serial3;
    case 2: return &Serial4;
    case 3: return &Serial5;
    case 4: return &Serial6;
    case 5: return &Serial7;
    case 6: return &Serial8;
    default: return nullptr;
  }
}

/**
 * Description: Initialize all RS422 ports to a given baud rate.
 * Inputs:
 * - baud: baud rate to use for every port.
 * Outputs: Initializes serial ports and stores handles.
 */
void Rs422Ports::begin(unsigned long baud) {
  for (uint8_t i = 0; i < RS422_PORT_COUNT; i++) {
    _ports[i].serial = pickSerialForIndex(i);
    if (_ports[i].serial) {
      _ports[i].serial->begin(baud);
    }
  }
}
