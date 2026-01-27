#pragma once
#include <Arduino.h>
#include <FlexCAN_T4.h>

/**
 * MKS Servo status data structure.
 * Holds current position, velocity, current, and error state.
 */
struct MksServoStatus {
  int32_t position = 0;      // Current position in pulses
  uint16_t velocity = 0;     // Current velocity in RPM
  uint8_t current = 0;       // Motor current (0-255)
  uint8_t errorCode = 0;     // Error flags
  bool valid = false;        // Data valid flag
  uint32_t lastUpdateMs = 0; // Timestamp of last update
};

class CanBus {
public:
  /**
   * Description: Initialize the CAN bus controller.
   * Inputs:
   * - bitrate: CAN bus bitrate in bits per second.
   * Outputs: Configures CAN2 (pins 0/1) with the provided bitrate.
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

  /**
   * Description: Process events in the CAN pipeline.
   * Inputs:
   * - none
   * Outputs: Returns event bitmap.
   */
  uint64_t events(void);

  /**
   * Description: Log CAN error counters when they change.
   * Inputs:
   * - out: stream to print to.
   * - nowMs: current time in milliseconds.
   * Outputs: Prints a line when counters change or errors persist.
   */
  void logErrorCounters(Stream &out, uint32_t nowMs);

  /**
   * Description: Print current CAN health status (error counters + ESR1).
   * Inputs:
   * - out: stream to print to.
   * Outputs: Prints a single status line plus active error flags.
   */
  void printHealth(Stream &out);

  /**
   * Description: Dump buffered RX frames to the provided stream.
   * Inputs:
   * - out: stream to print to (e.g., Serial).
   * - max: maximum frames to print this call (0 = drain all).
   * Outputs: Number of frames printed.
   */
  size_t dumpRxLog(Stream &out, size_t max = 8);

  /**
   * Description: Request status from MKS Servo.
   * Inputs:
   * - canId: CAN identifier of the servo
   * Outputs: Returns true if request was queued
   */
  bool requestMksServoStatus(uint16_t canId);

  /**
   * Description: Get cached status for an MKS Servo.
   * Inputs:
   * - canId: CAN identifier of the servo
   * - status: output status structure
   * Outputs: Returns true if status is available and valid
   */
  bool getMksServoStatus(uint16_t canId, MksServoStatus &status);

  /**
   * Description: Process received CAN frames.
   * Should be called regularly in main loop to handle RX messages.
   * Inputs: None
   * Outputs: None
   */
  void processRxFrames();

private:
  static void handleRxStatic(const CAN_message_t &msg);
  void enqueueRxLog(const CAN_message_t &msg);
  bool popRxLog(CAN_message_t &msg);
  bool readErrorCounters(uint32_t &esr1, uint16_t &ecr);

  // MKS Servo status tracking
  void handleMksServoResponse(const CAN_message_t &msg);
  uint8_t findServoIndex(uint16_t canId);
  uint8_t registerServo(uint16_t canId);

  static constexpr bool kPollRx =
#if defined(CANBUS_USE_INTERRUPTS)
    false;
#elif defined(CANBUS_USE_POLLING)
    true;
#else
    true;
#endif

  FlexCAN_T4<CAN2, RX_SIZE_256, TX_SIZE_16> _can;
  static CanBus* _instance;
  bool _haveErrorSnapshot = false;
  bool _pollRx = false;
  uint32_t _lastEsr1 = 0;
  uint16_t _lastEcr = 0;
  uint32_t _lastErrLogMs = 0;
  static constexpr uint32_t kErrLogMinPeriodMs = 500;
  static constexpr size_t kRxLogSize = 16;
  CAN_message_t _rxLog[kRxLogSize] = {};
  volatile uint8_t _rxLogHead = 0;
  volatile uint8_t _rxLogTail = 0;
  volatile bool _rxLogOverflow = false;

  // MKS Servo status tracking
  static constexpr size_t kMaxTrackedServos = 16;
  MksServoStatus _servoStatus[kMaxTrackedServos] = {};
  uint16_t _servoCanIds[kMaxTrackedServos] = {};
  uint8_t _servoCount = 0;
};
