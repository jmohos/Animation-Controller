#include "CanBus.h"
#include <cstring>

void CanBus::begin(uint32_t bitrate) {
  _can.begin();
  _can.setBaudRate(bitrate);
}

bool CanBus::send(uint32_t id, const uint8_t *data, uint8_t len) {
  if (len > 8) {
    len = 8;
  }
  CAN_message_t msg = {};
  msg.id = id;
  msg.len = len;
  msg.flags.extended = (id > 0x7FFu);
  if (data && len > 0) {
    std::memcpy(msg.buf, data, len);
  }
  return _can.write(msg);
}
