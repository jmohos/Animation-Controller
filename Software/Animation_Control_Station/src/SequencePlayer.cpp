#include "SequencePlayer.h"
#include <cstring>
#include <cstdlib>
#include <ctype.h>

static bool parseUint32(const char *text, uint32_t &value) {
  if (!text) {
    return false;
  }
  int base = 10;
  if (text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) {
    base = 16;
  }
  char *end = nullptr;
  value = strtoul(text, &end, base);
  return (end != text);
}

static bool parseInt32(const char *text, int32_t &value) {
  if (!text) {
    return false;
  }
  int base = 10;
  if (text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) {
    base = 16;
  }
  char *end = nullptr;
  long parsed = strtol(text, &end, base);
  if (end == text) {
    return false;
  }
  value = static_cast<int32_t>(parsed);
  return true;
}

static bool parseMode(const char *text, SequenceEvent::Mode &mode) {
  if (!text || *text == '\0') {
    mode = SequenceEvent::Mode::Position;
    return true;
  }
  while (*text == ' ' || *text == '\t') {
    text++;
  }
  char buf[8] = {};
  size_t len = 0;
  while (*text != '\0' && *text != ' ' && *text != '\t' && len < sizeof(buf) - 1) {
    buf[len++] = static_cast<char>(tolower(static_cast<unsigned char>(*text)));
    text++;
  }
  buf[len] = '\0';

  if (strcmp(buf, "pos") == 0 || strcmp(buf, "position") == 0 || strcmp(buf, "p") == 0 || strcmp(buf, "0") == 0) {
    mode = SequenceEvent::Mode::Position;
    return true;
  }
  if (strcmp(buf, "vel") == 0 || strcmp(buf, "velocity") == 0 || strcmp(buf, "v") == 0 || strcmp(buf, "1") == 0) {
    mode = SequenceEvent::Mode::Velocity;
    return true;
  }
  return false;
}

static bool readLine(FsFile &file, char *buf, size_t len) {
  size_t idx = 0;
  bool gotAny = false;
  while (file.available()) {
    int c = file.read();
    if (c < 0) {
      break;
    }
    gotAny = true;
    if (c == '\r') {
      continue;
    }
    if (c == '\n') {
      break;
    }
    if (idx + 1 < len) {
      buf[idx++] = static_cast<char>(c);
    }
  }
  buf[idx] = '\0';
  return gotAny;
}

static void stripInlineComment(char *line) {
  if (!line) {
    return;
  }
  for (char *c = line; *c != '\0'; ++c) {
    if (*c == '#') {
      *c = '\0';
      break;
    }
  }
}

static bool isSectionLine(const char *line, const char *section) {
  if (!line || !section) {
    return false;
  }
  if (line[0] == '#') {
    line++;
    while (*line == ' ' || *line == '\t') {
      line++;
    }
  }
  if (line[0] != '[') {
    return false;
  }
  const char *start = line + 1;
  const char *end = strchr(start, ']');
  if (!end) {
    return false;
  }
  size_t len = static_cast<size_t>(end - start);
  size_t i = 0;
  for (; i < len; i++) {
    char a = static_cast<char>(tolower(static_cast<unsigned char>(start[i])));
    char b = static_cast<char>(tolower(static_cast<unsigned char>(section[i])));
    if (b == '\0' || a != b) {
      return false;
    }
  }
  return section[i] == '\0';
}

static int compareEvents(const void *a, const void *b) {
  const SequenceEvent *ea = static_cast<const SequenceEvent*>(a);
  const SequenceEvent *eb = static_cast<const SequenceEvent*>(b);
  if (ea->timeMs < eb->timeMs) return -1;
  if (ea->timeMs > eb->timeMs) return 1;
  if (ea->endpointId < eb->endpointId) return -1;
  if (ea->endpointId > eb->endpointId) return 1;
  return 0;
}

static bool resolveEndpoint(const AppConfig &config, uint8_t endpointIndex, const EndpointConfig *&ep, uint8_t &portIndex) {
  if (endpointIndex >= MAX_ENDPOINTS) {
    return false;
  }
  const EndpointConfig &candidate = config.endpoints[endpointIndex];
  if (!candidate.enabled) {
    return false;
  }
  if (candidate.serialPort < 1 || candidate.serialPort > 8) {
    return false;
  }
  if (candidate.motor < 1 || candidate.motor > 2) {
    return false;
  }
  ep = &candidate;
  portIndex = static_cast<uint8_t>(candidate.serialPort - 1);
  return true;
}

bool SequencePlayer::loadFromAnimation(SdCardManager &sd, const char *path, Stream &out) {
  _eventCount = 0;
  _loopMs = 0;
  _loaded = false;
  _nextIndex = 0;
  _lastTimeMs = 0;

  FsFile file;
  if (!sd.openFile(path, file)) {
    out.printf("SEQ: missing %s\n", path);
    return false;
  }

  bool inSequence = false;
  bool sawSequence = false;
  char line[160] = {};
  while (readLine(file, line, sizeof(line))) {
    char *cursor = line;
    while (*cursor == ' ' || *cursor == '\t') {
      cursor++;
    }
    if (*cursor == '\0') {
      continue;
    }
    if (isSectionLine(cursor, "sequence")) {
      inSequence = true;
      sawSequence = true;
      continue;
    }
    if (isSectionLine(cursor, "endpoints")) {
      inSequence = false;
      continue;
    }

    stripInlineComment(cursor);
    while (*cursor == ' ' || *cursor == '\t') {
      cursor++;
    }
    if (*cursor == '\0' || *cursor == '#') {
      continue;
    }
    if (!inSequence) {
      continue;
    }

    char *token = strtok(cursor, ",");
    char *tokens[6] = {};
    uint8_t count = 0;
    while (token && count < 6) {
      tokens[count++] = token;
      token = strtok(nullptr, ",");
    }
    if (count < 5) {
      out.printf("SEQ: skip line (need 5 fields): %s\n", line);
      continue;
    }

    uint32_t timeMs = 0;
    uint32_t endpointId = 0;
    int32_t position = 0;
    uint32_t velocity = 0;
    uint32_t accel = 0;
    SequenceEvent::Mode mode = SequenceEvent::Mode::Position;
    if (!parseUint32(tokens[0], timeMs) ||
        !parseUint32(tokens[1], endpointId) ||
        !parseInt32(tokens[2], position) ||
        !parseUint32(tokens[3], velocity) ||
        !parseUint32(tokens[4], accel)) {
      out.printf("SEQ: parse error: %s\n", line);
      continue;
    }
    if (count >= 6) {
      if (!parseMode(tokens[5], mode)) {
        out.printf("SEQ: invalid mode: %s\n", line);
        continue;
      }
    }

    if (timeMs > 300000) {
      out.printf("SEQ: time out of range: %lu\n", (unsigned long)timeMs);
      continue;
    }
    if (endpointId == 0 || endpointId > MAX_ENDPOINTS) {
      out.printf("SEQ: invalid endpoint %lu\n", (unsigned long)endpointId);
      continue;
    }

    if (_eventCount < kMaxEvents) {
      SequenceEvent &ev = _events[_eventCount++];
      ev.timeMs = timeMs;
      ev.endpointId = static_cast<uint8_t>(endpointId);
      ev.position = position;
      ev.velocity = velocity;
      ev.accel = accel;
      ev.mode = mode;
      if (timeMs > _loopMs) {
        _loopMs = timeMs;
      }
    } else {
      out.println("SEQ: event buffer full");
      break;
    }
  }

  file.close();
  if (!sawSequence) {
    out.println("SEQ: no [sequence] section found");
    return false;
  }
  if (_eventCount > 1) {
    qsort(_events, _eventCount, sizeof(SequenceEvent), compareEvents);
  }
  _loaded = (_eventCount > 0);
  return _loaded;
}

void SequencePlayer::reset() {
  _nextIndex = 0;
  _lastTimeMs = 0;
}

void SequencePlayer::update(uint32_t timeMs, RoboClawBus &bus, const AppConfig &config) {
  if (!_loaded || _eventCount == 0) {
    return;
  }
  uint32_t t = timeMs;
  if (_loopMs > 0) {
    const uint32_t loopLen = _loopMs + 1;
    t = timeMs % loopLen;
  }
  if (t < _lastTimeMs) {
    _nextIndex = 0;
  }
  _lastTimeMs = t;

  while (_nextIndex < _eventCount && _events[_nextIndex].timeMs <= t) {
    const SequenceEvent &ev = _events[_nextIndex];
    const EndpointConfig *ep = nullptr;
    uint8_t portIndex = 0;
    if (resolveEndpoint(config, static_cast<uint8_t>(ev.endpointId - 1), ep, portIndex)) {
      const uint32_t maxVel = (ep->maxVelocity > 0) ? ep->maxVelocity : 1;
      const uint32_t maxAcc = (ep->maxAccel > 0) ? ep->maxAccel : 1;
      uint32_t vel = (ev.velocity > maxVel) ? maxVel : ev.velocity;
      uint32_t acc = (ev.accel > maxAcc) ? maxAcc : ev.accel;
      if (ev.mode == SequenceEvent::Mode::Velocity) {
        bus.commandVelocity(portIndex, ep->address, ep->motor, vel, acc);
      } else {
        uint32_t pos = (ev.position < 0) ? 0u : static_cast<uint32_t>(ev.position);
        bus.commandPosition(portIndex, ep->address, ep->motor, pos, vel, acc);
      }
    }
    _nextIndex++;
  }
}
