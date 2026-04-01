#include "SdCard.h"
#include "ConfigStore.h"
#include "AnimComProtocol.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {
static constexpr uint8_t kEndpointCsvColumns = 5 + MAX_ENDPOINT_ITEMS;

bool splitEndpointCsv(char *line, char *tokens[], uint8_t maxTokens, uint8_t &count) {
  count = 0;
  char *token = strtok(line, ",");
  while (token && count < maxTokens) {
    while (*token == ' ' || *token == '\t') {
      token++;
    }
    tokens[count++] = token;
    token = strtok(nullptr, ",");
  }
  return count > 0;
}

bool validateEndpointConfigLine(const EndpointConfig &ep, uint8_t endpointId, Stream &out) {
  const uint8_t maxItems = endpointTypeMaxItems(ep.type);
  if (!isSupportedEndpointType(ep.type)) {
    out.printf("CFG: unsupported type on EP%u\n", (unsigned)endpointId);
    return false;
  }
  if (ep.stationId == 0 || ep.stationId == ANIMCOM_STATION_BCAST) {
    out.printf("CFG: invalid station on EP%u\n", (unsigned)endpointId);
    return false;
  }
  if (ep.itemCount > maxItems) {
    out.printf("CFG: too many items on EP%u\n", (unsigned)endpointId);
    return false;
  }

  for (uint8_t i = 0; i < MAX_ENDPOINT_ITEMS; i++) {
    const EndpointItemType itemType = ep.itemTypes[i];
    if (i >= ep.itemCount) {
      if (itemType != EndpointItemType::None) {
        out.printf("CFG: EP%u item%u must be NONE\n", (unsigned)endpointId, (unsigned)(i + 1));
        return false;
      }
      continue;
    }

    if (ep.type == EndpointType::AnimComOctal && itemType != EndpointItemType::Motor) {
      out.printf("CFG: octal items must be MOTOR on EP%u\n", (unsigned)endpointId);
      return false;
    }
    if (ep.type == EndpointType::AnimComQuad) {
      if (i < 2 && itemType != EndpointItemType::Motor) {
        out.printf("CFG: quad items 1-2 must be MOTOR on EP%u\n", (unsigned)endpointId);
        return false;
      }
      if (i >= 2 && itemType != EndpointItemType::Servo) {
        out.printf("CFG: quad items 3-4 must be SERVO on EP%u\n", (unsigned)endpointId);
        return false;
      }
    }
    if (ep.type == EndpointType::Audio && itemType != EndpointItemType::AudioTrack) {
      out.printf("CFG: audio items must be AUDIO on EP%u\n", (unsigned)endpointId);
      return false;
    }
  }

  return true;
}
}

bool SdCardManager::begin() {
  _ready = _sd.begin(SdioConfig(FIFO_SDIO));
  return _ready;
}

bool SdCardManager::listDir(const char *path, Stream &out) {
  if (!_ready) {
    return false;
  }
  FsFile dir = _sd.open(path);
  if (!dir || !dir.isDir()) {
    return false;
  }

  out.printf("DIR %s\n", path);
  while (true) {
    FsFile entry = dir.openNextFile();
    if (!entry) {
      break;
    }
    char name[64] = {};
    entry.getName(name, sizeof(name));
    if (entry.isDir()) {
      out.printf("  [DIR] %s\n", name);
    } else {
      out.printf("  %s (%lu bytes)\n", name, (unsigned long)entry.size());
    }
    entry.close();
  }
  dir.close();
  return true;
}

bool SdCardManager::readFile(const char *path, Stream &out) {
  if (!_ready) {
    return false;
  }
  FsFile file = _sd.open(path, O_RDONLY);
  if (!file) {
    return false;
  }
  out.printf("READ %s\n", path);
  uint8_t buf[64];
  while (true) {
    int count = file.read(buf, sizeof(buf));
    if (count <= 0) {
      break;
    }
    out.write(buf, (size_t)count);
  }
  out.println();
  file.close();
  return true;
}

bool SdCardManager::testReadWrite(Stream &out) {
  if (!_ready) {
    return false;
  }

  const char *path = "/sd_test.txt";
  const char *payload = "SD CARD TEST OK\n";

  FsFile file = _sd.open(path, O_WRITE | O_CREAT | O_TRUNC);
  if (!file) {
    return false;
  }
  size_t written = file.write(payload, strlen(payload));
  file.close();
  if (written != strlen(payload)) {
    return false;
  }

  FsFile readback = _sd.open(path, O_RDONLY);
  if (!readback) {
    return false;
  }
  char buf[32] = {};
  int count = readback.read(buf, sizeof(buf) - 1);
  readback.close();
  if (count <= 0) {
    return false;
  }

  out.printf("SD TEST: wrote %u bytes, read %d bytes\n", (unsigned)written, count);
  out.printf("SD TEST: %s", buf);
  return true;
}

bool SdCardManager::loadEndpointConfig(AppConfig &cfg, Stream &out) {
  if (!_ready) {
    return false;
  }
  FsFile file = _sd.open(kEndpointConfigPath, O_RDONLY);
  if (!file) {
    return false;
  }

  char line[192] = {};
  while (readLine(file, line, sizeof(line))) {
    stripInlineComment(line);
    char *cursor = line;
    while (*cursor == ' ' || *cursor == '\t') {
      cursor++;
    }
    if (*cursor == '\0') {
      continue;
    }

    char *tokens[kEndpointCsvColumns] = {};
    uint8_t count = 0;
    if (!splitEndpointCsv(cursor, tokens, kEndpointCsvColumns, count)) {
      continue;
    }
    if (count != kEndpointCsvColumns) {
      out.printf("CFG: skip line (need %u fields): %s\n", (unsigned)kEndpointCsvColumns, line);
      continue;
    }

    uint32_t endpointId = 0;
    EndpointConfig ep = {};
    uint32_t stationId = 0;
    uint32_t enabled = 0;
    uint32_t itemCount = 0;
    if (!parseUint(tokens[0], endpointId) ||
        !parseEndpointType(tokens[1], ep.type) ||
        !parseUint(tokens[2], stationId) ||
        !parseUint(tokens[3], enabled) ||
        !parseUint(tokens[4], itemCount)) {
      out.printf("CFG: parse error: %s\n", line);
      continue;
    }

    if (endpointId == 0 || endpointId > MAX_ENDPOINTS) {
      out.printf("CFG: invalid endpoint %lu\n", (unsigned long)endpointId);
      continue;
    }

    ep.stationId = static_cast<uint8_t>(stationId);
    ep.enabled = (enabled != 0) ? 1 : 0;
    ep.itemCount = static_cast<uint8_t>(itemCount);
    for (uint8_t i = 0; i < MAX_ENDPOINT_ITEMS; i++) {
      EndpointItemType itemType = EndpointItemType::None;
      if (!parseEndpointItemType(tokens[5 + i], itemType)) {
        out.printf("CFG: invalid item type on EP%lu item%u\n",
                   (unsigned long)endpointId, (unsigned)(i + 1));
        itemType = EndpointItemType::None;
      }
      ep.itemTypes[i] = itemType;
    }

    if (!validateEndpointConfigLine(ep, static_cast<uint8_t>(endpointId), out)) {
      continue;
    }

    cfg.endpoints[endpointId - 1] = ep;
  }

  file.close();
  return true;
}

bool SdCardManager::saveEndpointConfig(const AppConfig &cfg, Stream &out) {
  if (!_ready) {
    return false;
  }
  FsFile file = _sd.open(kEndpointConfigPath, O_WRITE | O_CREAT | O_TRUNC);
  if (!file) {
    return false;
  }
  writeEndpointsSection(file, cfg);
  file.close();
  out.printf("CFG: wrote %s\n", kEndpointConfigPath);
  return true;
}

bool SdCardManager::loadAnimationConfig(const char *path, AppConfig &cfg, Stream &out) {
  (void)path;
  (void)cfg;
  out.println("ANIM: sequence playback removed");
  return false;
}

bool SdCardManager::saveAnimationConfig(const char *path, const AppConfig &cfg, Stream &out) {
  (void)path;
  (void)cfg;
  out.println("ANIM: sequence playback removed");
  return false;
}

bool SdCardManager::saveDefaultAnimation(const char *path, Stream &out) {
  (void)path;
  out.println("ANIM: sequence playback removed");
  return false;
}

bool SdCardManager::updateAnimationConfig(const char *path, const AppConfig &cfg, Stream &out) {
  (void)path;
  (void)cfg;
  out.println("ANIM: sequence playback removed");
  return false;
}

bool SdCardManager::readLine(FsFile &file, char *buf, size_t len) {
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

bool SdCardManager::parseInt(const char *token, int32_t &value) {
  if (!token) {
    return false;
  }
  while (*token == ' ' || *token == '\t') {
    token++;
  }
  if (*token == '\0') {
    return false;
  }
  int base = 10;
  if (token[0] == '0' && (token[1] == 'x' || token[1] == 'X')) {
    base = 16;
  }
  char *end = nullptr;
  long parsed = strtol(token, &end, base);
  if (end == token) {
    return false;
  }
  value = static_cast<int32_t>(parsed);
  return true;
}

bool SdCardManager::parseUint(const char *token, uint32_t &value) {
  if (!token) {
    return false;
  }
  while (*token == ' ' || *token == '\t') {
    token++;
  }
  if (*token == '\0') {
    return false;
  }
  int base = 10;
  if (token[0] == '0' && (token[1] == 'x' || token[1] == 'X')) {
    base = 16;
  }
  char *end = nullptr;
  value = strtoul(token, &end, base);
  return (end != token);
}

void SdCardManager::stripInlineComment(char *line) const {
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

void SdCardManager::writeEndpointsSection(FsFile &file, const AppConfig &cfg) {
  file.println("# endpoint_id,type,station_id,enabled,item_count,item1,item2,item3,item4,item5,item6,item7,item8");
  file.println("# Supported endpoint types: ANIMCOM_OCTAL, ANIMCOM_QUAD, AUDIO");
  file.println("# Supported item types: MOTOR, SERVO, AUDIO, NONE");
  for (uint8_t i = 0; i < MAX_ENDPOINTS; i++) {
    const EndpointConfig &ep = cfg.endpoints[i];
    file.printf("%u,%s,%u,%u,%u",
                (unsigned)(i + 1),
                endpointTypeName(ep.type),
                (unsigned)ep.stationId,
                (unsigned)ep.enabled,
                (unsigned)ep.itemCount);
    for (uint8_t item = 0; item < MAX_ENDPOINT_ITEMS; item++) {
      file.printf(",%s", endpointItemTypeName(ep.itemTypes[item]));
    }
    file.println();
  }
}

bool SdCardManager::openFile(const char *path, FsFile &file) {
  if (!_ready) {
    return false;
  }
  file = _sd.open(path, O_RDONLY);
  return static_cast<bool>(file);
}

bool SdCardManager::openFileWrite(const char *path, FsFile &file) {
  if (!_ready) {
    return false;
  }
  file = _sd.open(path, O_WRITE | O_CREAT | O_TRUNC);
  return static_cast<bool>(file);
}
