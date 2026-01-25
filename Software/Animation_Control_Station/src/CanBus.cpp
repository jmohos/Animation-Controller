#include "CanBus.h"
#include "BoardPins.h"
#include <imxrt.h>
#include <cstring>

CanBus* CanBus::_instance = nullptr;

void CanBus::begin(uint32_t bitrate) {
  _instance = this;
  //pinMode(PIN_CAN_RX, INPUT_PULLUP);
  //pinMode(PIN_CAN_TX, OUTPUT);
  _pollRx = kPollRx;
  _can.begin();
  _can.setBaudRate(bitrate, TX);
  _can.setMaxMB(16);
  _can.enableFIFO();
  if (_pollRx) {
    _can.disableFIFOInterrupt();
    NVIC_DISABLE_IRQ(IRQ_CAN2);
  } else {
    _can.onReceive(handleRxStatic);
    _can.events(); // mark events used before enabling interrupts
    _can.enableFIFOInterrupt();
  }
  _can.mailboxStatus();
}

void CanBus::handleRxStatic(const CAN_message_t &msg)
{
  if (_instance) {
    _instance->enqueueRxLog(msg);
  }
}

void CanBus::enqueueRxLog(const CAN_message_t &msg) {
  const uint8_t next = static_cast<uint8_t>((_rxLogHead + 1) % kRxLogSize);
  if (next == _rxLogTail) {
    _rxLogOverflow = true;
    return;
  }
  _rxLog[_rxLogHead] = msg;
  _rxLogHead = next;
}

bool CanBus::popRxLog(CAN_message_t &msg) {
  noInterrupts();
  if (_rxLogTail == _rxLogHead) {
    interrupts();
    return false;
  }
  msg = _rxLog[_rxLogTail];
  _rxLogTail = static_cast<uint8_t>((_rxLogTail + 1) % kRxLogSize);
  interrupts();
  return true;
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
  return (_can.write(msg) > 0);
}

uint64_t CanBus::events()
{
  if (_pollRx) {
    CAN_message_t msg;
    while (_can.read(msg) > 0) {
      enqueueRxLog(msg);
    }
    return (static_cast<uint64_t>(_can.getRXQueueCount()) << 12) | _can.getTXQueueCount();
  }
  return (_can.events());
}

bool CanBus::readErrorCounters(uint32_t &esr1, uint16_t &ecr) {
  esr1 = FLEXCAN2_ESR1;
  ecr = static_cast<uint16_t>(FLEXCAN2_ECR);
  return true;
}

static const char *faultStateName(uint8_t faultCode) {
  switch (faultCode) {
    case 0: return "Error Active";
    case 1: return "Error Passive";
    case 2: return "Bus Off";
    default: return "Unknown";
  }
}

void CanBus::logErrorCounters(Stream &out, uint32_t nowMs) {
  uint32_t esr1 = 0;
  uint16_t ecr = 0;
  if (!readErrorCounters(esr1, ecr)) {
    return;
  }

  const bool changed = (esr1 != _lastEsr1) || (ecr != _lastEcr);
  const bool periodic = (nowMs - _lastErrLogMs) >= kErrLogMinPeriodMs;
  const bool hasErrors = (ecr != 0) || (esr1 & 0x0003F000u);

  if (changed || (hasErrors && periodic)) {
    const uint8_t rxErr = static_cast<uint8_t>((ecr >> 8) & 0xFFu);
    const uint8_t txErr = static_cast<uint8_t>(ecr & 0xFFu);
    const uint8_t fault = static_cast<uint8_t>((esr1 & FLEXCAN_ESR_FLT_CONF_MASK) >> 4);
    out.printf("CAN ERR: ESR1=0x%08lX ECR=0x%04X RX=%u TX=%u %s\n",
               static_cast<unsigned long>(esr1),
               static_cast<unsigned>(ecr),
               static_cast<unsigned>(rxErr),
               static_cast<unsigned>(txErr),
               faultStateName(fault));
    _lastEsr1 = esr1;
    _lastEcr = ecr;
    _lastErrLogMs = nowMs;
  }
}

void CanBus::printHealth(Stream &out) {
  uint32_t esr1 = 0;
  uint16_t ecr = 0;
  if (!readErrorCounters(esr1, ecr)) {
    out.println("CAN: health unavailable");
    return;
  }

  const uint8_t rxErr = static_cast<uint8_t>((ecr >> 8) & 0xFFu);
  const uint8_t txErr = static_cast<uint8_t>(ecr & 0xFFu);
  const uint8_t fault = static_cast<uint8_t>((esr1 & FLEXCAN_ESR_FLT_CONF_MASK) >> 4);
  out.printf("CAN: ESR1=0x%08lX ECR=0x%04X RX=%u TX=%u %s",
             static_cast<unsigned long>(esr1),
             static_cast<unsigned>(ecr),
             static_cast<unsigned>(rxErr),
             static_cast<unsigned>(txErr),
             faultStateName(fault));

  if (esr1 & FLEXCAN_ESR_ACK_ERR) out.print(" ACK_ERR");
  if (esr1 & FLEXCAN_ESR_CRC_ERR) out.print(" CRC_ERR");
  if (esr1 & FLEXCAN_ESR_FRM_ERR) out.print(" FRM_ERR");
  if (esr1 & FLEXCAN_ESR_STF_ERR) out.print(" STF_ERR");
  if (esr1 & FLEXCAN_ESR_BIT0_ERR) out.print(" BIT0_ERR");
  if (esr1 & FLEXCAN_ESR_BIT1_ERR) out.print(" BIT1_ERR");
  if (esr1 & FLEXCAN_ESR_TX_WRN) out.print(" TX_WRN");
  if (esr1 & FLEXCAN_ESR_RX_WRN) out.print(" RX_WRN");
  out.print('\n');
}

size_t CanBus::dumpRxLog(Stream &out, size_t max) {
  bool overflow = false;
  noInterrupts();
  if (_rxLogOverflow) {
    overflow = true;
    _rxLogOverflow = false;
  }
  interrupts();

  if (overflow) {
    out.println("CAN RX LOG OVERFLOW");
  }

  size_t count = 0;
  CAN_message_t msg;
  while ((max == 0 || count < max) && popRxLog(msg)) {
    out.print("MB ");
    out.print(msg.mb);
    out.print("  OVERRUN: ");
    out.print(msg.flags.overrun);
    out.print("  LEN: ");
    out.print(msg.len);
    out.print(" EXT: ");
    out.print(msg.flags.extended);
    out.print(" TS: ");
    out.print(msg.timestamp);
    out.print(" ID: ");
    out.print(msg.id, HEX);
    out.print(" Buffer: ");
    for (uint8_t i = 0; i < msg.len; i++) {
      out.print(msg.buf[i], HEX);
      out.print(" ");
    }
    out.println();
    count++;
  }
  return count;
}
