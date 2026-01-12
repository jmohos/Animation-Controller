#pragma once
#include <Arduino.h>
#include "ConfigStore.h"
#include "RoboClawBus.h"
#include "SdCard.h"

struct SequenceEvent {
  enum class Mode : uint8_t {
    Position = 0,
    Velocity = 1
  };

  uint32_t timeMs = 0;
  uint8_t endpointId = 0;
  int32_t position = 0;
  uint32_t velocity = 0;
  uint32_t accel = 0;
  Mode mode = Mode::Position;
};

class SequencePlayer {
public:
  static constexpr size_t kMaxEvents = 512;

  /**
   * Description: Load sequence events from an animation file.
   * Inputs:
   * - sd: SD card manager.
   * - path: animation file path.
   * - out: output stream for status.
   * Outputs: Returns true when sequence events are loaded.
   */
  bool loadFromAnimation(SdCardManager &sd, const char *path, Stream &out);

  /**
   * Description: Reset playback state.
   * Inputs: None.
   * Outputs: Resets cursor to start of the sequence.
   */
  void reset();

  /**
   * Description: Dispatch due sequence events for the current time.
   * Inputs:
   * - timeMs: current show time in milliseconds.
   * - bus: RoboClaw bus for command dispatch.
   * - config: endpoint configuration table.
   * Outputs: Sends motion commands for due events.
   */
  void update(uint32_t timeMs, RoboClawBus &bus, const AppConfig &config);

  size_t eventCount() const { return _eventCount; }
  uint32_t loopMs() const { return _loopMs; }
  bool loaded() const { return _loaded; }

private:
  SequenceEvent _events[kMaxEvents] = {};
  size_t _eventCount = 0;
  uint32_t _loopMs = 0;
  uint32_t _lastTimeMs = 0;
  size_t _nextIndex = 0;
  bool _loaded = false;
};
