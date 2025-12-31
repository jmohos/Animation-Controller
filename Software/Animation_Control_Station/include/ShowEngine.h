#pragma once
#include <Arduino.h>
#include "Timebase.h"

class ShowEngine {
public:
  void begin() { _tb.reset(); }
  void setPlaying(bool p) {
    if (p && !_playing) { _tb.reset(); _resumeOffsetMs = _pausedAtMs; }
    if (!p && _playing) { _pausedAtMs = currentTimeMs(); }
    _playing = p;
  }
  bool isPlaying() const { return _playing; }

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
