#pragma once
#include <Arduino.h>
#include "Timebase.h"

class ShowEngine {
public:
  /**
   * Description: Initialize the show timebase.
   * Inputs: None.
   * Outputs: Resets the internal timebase.
   */
  void begin() { _tb.reset(); }

  /**
   * Description: Set play/pause state and manage timing offsets.
   * Inputs:
   * - playing: true to play, false to pause.
   * Outputs: Updates internal play state and timing offsets.
   */
  void setPlaying(bool playing) {
    if (playing && !_playing) { _tb.reset(); _resumeOffsetMs = _pausedAtMs; }
    if (!playing && _playing) { _pausedAtMs = currentTimeMs(); }
    _playing = playing;
  }

  /**
   * Description: Check whether playback is active.
   * Inputs: None.
   * Outputs: Returns true when playing, false when paused.
   */
  bool isPlaying() const { return _playing; }

  /**
   * Description: Get the current show time in milliseconds.
   * Inputs: None.
   * Outputs: Returns elapsed or paused time based on play state.
   */
  uint32_t currentTimeMs() const {
    if (_playing) return _resumeOffsetMs + _tb.nowMs();
    return _pausedAtMs;
  }

private:
  Timebase _tb;
  bool _playing = false;
  uint32_t _pausedAtMs = 0;
  uint32_t _resumeOffsetMs = 0;
};
