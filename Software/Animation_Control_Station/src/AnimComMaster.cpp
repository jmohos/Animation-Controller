#include "AnimComMaster.h"

#include <Arduino.h>

void AnimComMaster::begin(HardwareSerial& port, uint8_t dePin, uint32_t baud)
{
    _port = &port;
    _dePin = dePin;
    _seq = 0;

    pinMode(_dePin, OUTPUT);
    digitalWrite(_dePin, LOW);

    _port->begin(baud);

    Serial.printf("[AnimComMaster] RS485 master ready  baud=%lu  dePin=%d\n",
                  (unsigned long)baud,
                  dePin);
}

void AnimComMaster::sendControlState(uint8_t stationId,
                                     uint8_t state,
                                     uint8_t pattern,
                                     uint8_t speedScale)
{
    AnimComFrame f = {};
    f.station_id = stationId;
    f.seq = _seq++;
    f.msg_type = ANIMCOM_MSG_CONTROL_STATE;
    f.payload_len = 3;
    f.payload[0] = state;
    f.payload[1] = pattern;
    f.payload[2] = speedScale;

    uint8_t buf[ANIMCOM_MAX_FRAME];
    uint8_t len = animcom_build_frame(buf, sizeof(buf), &f);
    _transmit(buf, len);
}

void AnimComMaster::sendTriggerEffect(uint8_t stationId,
                                      uint8_t effectType,
                                      uint8_t effectId,
                                      uint16_t effectParam)
{
    AnimComFrame f = {};
    f.station_id = stationId;
    f.seq = _seq++;
    f.msg_type = ANIMCOM_MSG_TRIGGER_EFFECT;
    f.payload_len = 4;
    f.payload[0] = effectType;
    f.payload[1] = effectId;
    f.payload[2] = (uint8_t)(effectParam & 0xFFu);
    f.payload[3] = (uint8_t)(effectParam >> 8u);

    uint8_t buf[ANIMCOM_MAX_FRAME];
    uint8_t len = animcom_build_frame(buf, sizeof(buf), &f);
    _transmit(buf, len);
}

void AnimComMaster::sendManualSingle(uint8_t stationId,
                                     uint8_t channel,
                                     uint8_t cmdType,
                                     int32_t value)
{
    AnimComFrame f = {};
    f.station_id = stationId;
    f.seq = _seq++;
    f.msg_type = ANIMCOM_MSG_MANUAL_SINGLE;
    f.payload_len = 6;
    f.payload[0] = channel;
    f.payload[1] = cmdType;
    f.payload[2] = (uint8_t)(value & 0xFFu);
    f.payload[3] = (uint8_t)((value >> 8) & 0xFFu);
    f.payload[4] = (uint8_t)((value >> 16) & 0xFFu);
    f.payload[5] = (uint8_t)((value >> 24) & 0xFFu);

    uint8_t buf[ANIMCOM_MAX_FRAME];
    uint8_t len = animcom_build_frame(buf, sizeof(buf), &f);
    _transmit(buf, len);
}

void AnimComMaster::sendManualBulk(uint8_t stationId, const int8_t speeds[ANIMCOM_BULK_MOTORS])
{
    AnimComFrame f = {};
    f.station_id = stationId;
    f.seq = _seq++;
    f.msg_type = ANIMCOM_MSG_MANUAL_BULK;
    f.payload_len = ANIMCOM_BULK_MOTORS;
    for (uint8_t i = 0; i < ANIMCOM_BULK_MOTORS; i++) {
        f.payload[i] = (uint8_t)speeds[i];
    }

    uint8_t buf[ANIMCOM_MAX_FRAME];
    uint8_t len = animcom_build_frame(buf, sizeof(buf), &f);
    _transmit(buf, len);
}

void AnimComMaster::_transmit(const uint8_t* buf, uint8_t len)
{
    if (!_port || len == 0) {
        return;
    }

    digitalWrite(_dePin, HIGH);
    _port->write(buf, len);
    _port->flush();
    delayMicroseconds(DE_SETTLE_US);
    digitalWrite(_dePin, LOW);

    _txFrames++;
}
