#include "SequencePlayer.h"
#include "BoardPins.h"
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

static uint32_t clampU32Range(uint32_t value, uint32_t minValue, uint32_t maxValue) {
  if (maxValue > 0) {
    if (maxValue < minValue) {
      minValue = maxValue;
    }
    if (value < minValue) {
      return minValue;
    }
    if (value > maxValue) {
      return maxValue;
    }
  } else if (minValue > 0 && value < minValue) {
    return minValue;
  }
  return value;
}

static int32_t clampI32Range(int32_t value, int32_t minValue, int32_t maxValue) {
  if (maxValue > minValue) {
    if (value < minValue) {
      return minValue;
    }
    if (value > maxValue) {
      return maxValue;
    }
  }
  return value;
}

static bool resolveEndpoint(const AppConfig &config, uint8_t endpointIndex, const EndpointConfig *&ep, uint8_t &portIndex) {
  if (endpointIndex >= MAX_ENDPOINTS) {
    return false;
  }
  const EndpointConfig &candidate = config.endpoints[endpointIndex];
  if (!candidate.enabled) {
    return false;
  }
  if (candidate.type != EndpointType::RoboClaw) {
    return false;
  }
  if (candidate.serialPort < 1 || candidate.serialPort > RS422_PORT_COUNT) {
    return false;
  }
  if (candidate.motor < 1 || candidate.motor > 2) {
    return false;
  }
  if (candidate.address > 0xFF) {
    return false;
  }
  ep = &candidate;
  portIndex = static_cast<uint8_t>(candidate.serialPort - 1);
  return true;
}

static bool isCanEndpointType(EndpointType type) {
  return (type == EndpointType::MksServo ||
          type == EndpointType::RevFrcCan ||
          type == EndpointType::JoeServoCan);
}

static uint8_t mksServoChecksum(uint16_t canId, const uint8_t *data, uint8_t len) {
  // MKS CAN checksum: sum of CAN ID (low byte) + data bytes, truncated to 8 bits.
  uint32_t sum = static_cast<uint8_t>(canId & 0xFFu);
  for (uint8_t i = 0; i < len; i++) {
    sum += data[i];
  }
  return static_cast<uint8_t>(sum & 0xFFu);
}

static uint32_t encodeMksServoInt24(int32_t value) {
  const int32_t maxPos = 0x7FFFFF;
  if (value > maxPos) {
    value = maxPos;
  } else if (value < -maxPos) {
    value = -maxPos;
  }
  if (value < 0) {
    return static_cast<uint32_t>((1u << 24) + value);
  }
  return static_cast<uint32_t>(value);
}

static bool packMksServoPosition(uint16_t canId, uint16_t speed, uint8_t accel, int32_t position, uint8_t data[8]) {
  uint32_t axis = encodeMksServoInt24(position);
  data[0] = 0xF5;
  data[1] = static_cast<uint8_t>((speed >> 8) & 0xFFu);
  data[2] = static_cast<uint8_t>(speed & 0xFFu);
  data[3] = accel;
  data[4] = static_cast<uint8_t>((axis >> 16) & 0xFFu);
  data[5] = static_cast<uint8_t>((axis >> 8) & 0xFFu);
  data[6] = static_cast<uint8_t>(axis & 0xFFu);
  data[7] = mksServoChecksum(canId, data, 7);
  return true;
}

static bool packMksServoSpeed(uint16_t canId, uint16_t speed, uint8_t accel, bool reverse, uint8_t data[5]) {
  const uint8_t dir = reverse ? 0x80u : 0x00u;
  data[0] = 0xF6;
  data[1] = static_cast<uint8_t>(dir | ((speed >> 8) & 0x0Fu));
  data[2] = static_cast<uint8_t>(speed & 0xFFu);
  data[3] = accel;
  data[4] = mksServoChecksum(canId, data, 4);
  return true;
}

static void dispatchMksServoEvent(const SequenceEvent &ev, CanBus &can, const EndpointConfig &endpoint) {
  if (endpoint.address > 0x7FFu) {
    return;
  }
  const uint16_t canId = static_cast<uint16_t>(endpoint.address);
  uint32_t speedRaw = clampU32Range(ev.velocity, endpoint.velocityMin, endpoint.velocityMax);
  if (speedRaw > 3000u) {
    speedRaw = 3000u;
  }
  uint32_t accelRaw = clampU32Range(ev.accel, endpoint.accelMin, endpoint.accelMax);
  if (accelRaw > 255u) {
    accelRaw = 255u;
  }
  const uint16_t speed = static_cast<uint16_t>(speedRaw);
  const uint8_t accel = static_cast<uint8_t>(accelRaw);

  if (ev.mode == SequenceEvent::Mode::Velocity) {
    const bool reverse = (ev.position < 0);
    uint8_t data[5] = {};
    if (packMksServoSpeed(canId, speed, accel, reverse, data)) {
      can.send(canId, data, sizeof(data));
    }
  } else {
    int32_t pos = clampI32Range(ev.position, endpoint.positionMin, endpoint.positionMax);
    uint8_t data[8] = {};
    if (packMksServoPosition(canId, speed, accel, pos, data)) {
      can.send(canId, data, sizeof(data));
    }
  }
}

static void dispatchCanEvent(const SequenceEvent &ev, CanBus &can, const EndpointConfig &endpoint) {
  switch (endpoint.type) {
    case EndpointType::MksServo:
      dispatchMksServoEvent(ev, can, endpoint);
      break;
    default:
      break;
  }
}

static void dispatchEvent(const SequenceEvent &ev, RoboClawBus &roboclaw, CanBus &can, const AppConfig &config) {
  if (ev.endpointId == 0 || ev.endpointId > MAX_ENDPOINTS) {
    return;
  }
  const EndpointConfig &endpoint = config.endpoints[ev.endpointId - 1];
  if (!endpoint.enabled) {
    return;
  }

  if (endpoint.type == EndpointType::RoboClaw) {
    const EndpointConfig *ep = nullptr;
    uint8_t portIndex = 0;
    if (resolveEndpoint(config, static_cast<uint8_t>(ev.endpointId - 1), ep, portIndex)) {
      uint32_t vel = clampU32Range(ev.velocity, ep->velocityMin, ep->velocityMax);
      uint32_t acc = clampU32Range(ev.accel, ep->accelMin, ep->accelMax);
      if (ev.mode == SequenceEvent::Mode::Velocity) {
        roboclaw.commandVelocity(portIndex, static_cast<uint8_t>(ep->address), ep->motor, vel, acc);
      } else {
        int32_t pos = clampI32Range(ev.position, ep->positionMin, ep->positionMax);
        uint32_t posU = static_cast<uint32_t>(pos);
        roboclaw.commandPosition(portIndex, static_cast<uint8_t>(ep->address), ep->motor, posU, vel, acc);
      }
    }
  } else if (isCanEndpointType(endpoint.type)) {
    dispatchCanEvent(ev, can, endpoint);
  }
}

static const char *modeName(SequenceEvent::Mode mode) {
  return (mode == SequenceEvent::Mode::Velocity) ? "vel" : "pos";
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
    sortEvents();
  }
  _loaded = (_eventCount > 0);
  return _loaded;
}

void SequencePlayer::reset() {
  _nextIndex = 0;
  _lastTimeMs = 0;
}

bool SequencePlayer::saveToAnimation(SdCardManager &sd, const char *path, Stream &out) const {
  FsFile file;
  if (!sd.openFileWrite(path, file)) {
    out.printf("SEQ: write failed %s\n", path);
    return false;
  }
  file.println("[sequence]");
  file.println("# time_ms,endpoint_id,position,velocity,accel,mode");
  for (size_t i = 0; i < _eventCount; i++) {
    const SequenceEvent &ev = _events[i];
    file.printf("%lu,%u,%ld,%lu,%lu,%s\n",
                (unsigned long)ev.timeMs,
                (unsigned)ev.endpointId,
                (long)ev.position,
                (unsigned long)ev.velocity,
                (unsigned long)ev.accel,
                modeName(ev.mode));
  }
  file.close();
  out.printf("SEQ: wrote %s\n", path);
  return true;
}

bool SequencePlayer::getEvent(size_t index, SequenceEvent &event) const {
  if (index >= _eventCount) {
    return false;
  }
  event = _events[index];
  return true;
}

bool SequencePlayer::setEvent(size_t index, const SequenceEvent &event, size_t *newIndex, bool keepOrder) {
  if (index >= _eventCount) {
    return false;
  }
  _events[index] = event;
  if (keepOrder) {
    recomputeLoopMs();
    _nextIndex = 0;
    _lastTimeMs = 0;
    _loaded = (_eventCount > 0);
    if (newIndex) {
      *newIndex = index;
    }
    return true;
  }

  sortEvents();
  recomputeLoopMs();
  _nextIndex = 0;
  _lastTimeMs = 0;
  _loaded = (_eventCount > 0);
  if (newIndex) {
    size_t found = findEventIndex(event);
    *newIndex = (found < _eventCount) ? found : index;
  }
  return true;
}

bool SequencePlayer::insertEvent(const SequenceEvent &event, size_t *newIndex) {
  if (_eventCount >= kMaxEvents) {
    return false;
  }
  _events[_eventCount++] = event;
  sortEvents();
  recomputeLoopMs();
  _nextIndex = 0;
  _lastTimeMs = 0;
  _loaded = true;
  if (newIndex) {
    size_t found = findEventIndex(event);
    *newIndex = (found < _eventCount) ? found : (_eventCount - 1);
  }
  return true;
}

bool SequencePlayer::deleteEvent(size_t index) {
  if (index >= _eventCount) {
    return false;
  }
  for (size_t i = index + 1; i < _eventCount; i++) {
    _events[i - 1] = _events[i];
  }
  _eventCount--;
  recomputeLoopMs();
  _nextIndex = 0;
  _lastTimeMs = 0;
  _loaded = (_eventCount > 0);
  return true;
}

void SequencePlayer::sortForPlayback() {
  sortEvents();
  recomputeLoopMs();
  _nextIndex = 0;
  _lastTimeMs = 0;
  _loaded = (_eventCount > 0);
}

void SequencePlayer::update(uint32_t timeMs, RoboClawBus &roboclaw, CanBus &can, const AppConfig &config) {
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

  bool pending[MAX_ENDPOINTS] = {};
  SequenceEvent lastEvent[MAX_ENDPOINTS] = {};
  while (_nextIndex < _eventCount && _events[_nextIndex].timeMs <= t) {
    const SequenceEvent &ev = _events[_nextIndex];
    if (ev.endpointId > 0 && ev.endpointId <= MAX_ENDPOINTS) {
      const uint8_t endpointIndex = static_cast<uint8_t>(ev.endpointId - 1);
      pending[endpointIndex] = true;
      lastEvent[endpointIndex] = ev;
    }
    _nextIndex++;
  }

  for (uint8_t i = 0; i < MAX_ENDPOINTS; i++) {
    if (!pending[i]) {
      continue;
    }
    dispatchEvent(lastEvent[i], roboclaw, can, config);
  }
}

void SequencePlayer::sortEvents() {
  if (_eventCount <= 1) {
    return;
  }
  // Stable insertion sort to preserve order for identical time/endpoint entries.
  for (size_t i = 1; i < _eventCount; i++) {
    SequenceEvent key = _events[i];
    size_t j = i;
    while (j > 0 && compareEvents(&key, &_events[j - 1]) < 0) {
      _events[j] = _events[j - 1];
      j--;
    }
    _events[j] = key;
  }
}

void SequencePlayer::recomputeLoopMs() {
  _loopMs = 0;
  for (size_t i = 0; i < _eventCount; i++) {
    if (_events[i].timeMs > _loopMs) {
      _loopMs = _events[i].timeMs;
    }
  }
}

size_t SequencePlayer::findEventIndex(const SequenceEvent &event) const {
  for (size_t i = 0; i < _eventCount; i++) {
    const SequenceEvent &candidate = _events[i];
    if (candidate.timeMs != event.timeMs ||
        candidate.endpointId != event.endpointId ||
        candidate.position != event.position ||
        candidate.velocity != event.velocity ||
        candidate.accel != event.accel ||
        candidate.mode != event.mode) {
      continue;
    }
    return i;
  }
  return _eventCount;
}
