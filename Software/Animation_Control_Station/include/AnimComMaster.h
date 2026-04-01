#pragma once

#include <Arduino.h>
#include "AnimComProtocol.h"

class AnimComMaster {
public:
    void begin(HardwareSerial& port, uint8_t dePin, uint32_t baud = ANIMCOM_BAUD);

    void sendControlState(uint8_t stationId,
                          uint8_t state,
                          uint8_t pattern = 0,
                          uint8_t speedScale = 100);

    void sendTriggerEffect(uint8_t stationId,
                           uint8_t effectType,
                           uint8_t effectId,
                           uint16_t effectParam = 0);

    void sendManualSingle(uint8_t stationId,
                          uint8_t channel,
                          uint8_t cmdType,
                          int32_t value);

    void sendManualBulk(uint8_t stationId, const int8_t speeds[ANIMCOM_BULK_MOTORS]);

    uint32_t getTxFrames() const { return _txFrames; }
    uint8_t getSeq() const { return _seq; }

private:
    static constexpr uint32_t ANIMCOM_BAUD = 115200;
    static constexpr uint32_t DE_SETTLE_US = 50;

    HardwareSerial* _port = nullptr;
    uint8_t _dePin = 0;
    uint8_t _seq = 0;
    uint32_t _txFrames = 0;

    void _transmit(const uint8_t* buf, uint8_t len);
};
