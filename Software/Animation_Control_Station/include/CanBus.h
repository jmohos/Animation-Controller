#pragma once
#include <Arduino.h>
#include <FlexCAN_T4.h>

class CanBus {
public:
  /**
   * Description: Initialize the CAN bus controller.
   * Inputs:
   * - bitrate: CAN bus bitrate in bits per second.
   * Outputs: Configures CAN2 with the provided bitrate.
   */
  void begin(uint32_t bitrate);

  /**
   * Description: Transmit a raw CAN frame.
   * Inputs:
   * - id: 11-bit or 29-bit CAN identifier.
   * - data: payload bytes (0-8 bytes).
   * - len: payload length.
   * Outputs: Returns true on successful queueing.
   */
  bool send(uint32_t id, const uint8_t *data, uint8_t len);

private:
  FlexCAN_T4<CAN2, RX_SIZE_256, TX_SIZE_16> _can;
};
