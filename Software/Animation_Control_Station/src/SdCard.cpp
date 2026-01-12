#include "SdCard.h"
#include "ConfigStore.h"
#include <cstring>
#include <cstdlib>
#include <ctype.h>
#include <cstdio>

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

  char line[128] = {};
  while (readLine(file, line, sizeof(line))) {
    stripInlineComment(line);
    char *cursor = line;
    while (*cursor == ' ' || *cursor == '\t') {
      cursor++;
    }
    if (*cursor == '\0' || *cursor == '#') {
      continue;
    }

    char *token = strtok(cursor, ",");
    char *tokens[8] = {};
    uint8_t count = 0;
    while (token && count < 8) {
      tokens[count++] = token;
      token = strtok(nullptr, ",");
    }
    if (count < 5) {
      out.printf("CFG: skip line (need 5 fields): %s\n", line);
      continue;
    }

    uint32_t endpointId = 0;
    uint32_t port = 0;
    uint32_t address = 0;
    uint32_t motor = 0;
    uint32_t enabled = 0;
    uint32_t maxVelocity = 0;
    uint32_t maxAccel = 0;
    bool hasVelocity = false;
    bool hasAccel = false;
    if (!parseUint(tokens[0], endpointId) ||
        !parseUint(tokens[1], port) ||
        !parseUint(tokens[2], address) ||
        !parseUint(tokens[3], motor) ||
        !parseUint(tokens[4], enabled)) {
      out.printf("CFG: parse error: %s\n", line);
      continue;
    }
    if (count == 6) {
      if (!parseUint(tokens[5], maxVelocity)) {
        out.printf("CFG: max_velocity parse error: %s\n", line);
        continue;
      }
      hasVelocity = true;
    } else if (count == 7) {
      if (!parseUint(tokens[5], maxVelocity) || !parseUint(tokens[6], maxAccel)) {
        out.printf("CFG: max_velocity/max_accel parse error: %s\n", line);
        continue;
      }
      hasVelocity = true;
      hasAccel = true;
    } else if (count >= 8) {
      if (!parseUint(tokens[6], maxVelocity) || !parseUint(tokens[7], maxAccel)) {
        out.printf("CFG: max_velocity/max_accel parse error: %s\n", line);
        continue;
      }
      hasVelocity = true;
      hasAccel = true;
    }

    if (endpointId == 0 || endpointId > MAX_ENDPOINTS) {
      out.printf("CFG: invalid endpoint %lu\n", (unsigned long)endpointId);
      continue;
    }

    EndpointConfig &ep = cfg.endpoints[endpointId - 1];
    if (!hasVelocity) {
      maxVelocity = ep.maxVelocity;
    }
    if (!hasAccel) {
      maxAccel = ep.maxAccel;
    }
    ep.serialPort = (port >= 1 && port <= 8) ? static_cast<uint8_t>(port) : ep.serialPort;
    ep.address = (address <= 0xFF) ? static_cast<uint8_t>(address) : ep.address;
    ep.motor = (motor == 1 || motor == 2) ? static_cast<uint8_t>(motor) : ep.motor;
    ep.enabled = (enabled != 0);
    ep.maxVelocity = maxVelocity;
    ep.maxAccel = maxAccel;
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
  file.println("# endpoint_id,serial_port,address,motor,enabled,max_velocity,max_accel");
  for (uint8_t i = 0; i < MAX_ENDPOINTS; i++) {
    const EndpointConfig &ep = cfg.endpoints[i];
    file.printf("%u,%u,0x%02X,%u,%u,%lu,%lu\n",
                (unsigned)(i + 1),
                (unsigned)ep.serialPort,
                (unsigned)ep.address,
                (unsigned)ep.motor,
                (unsigned)(ep.enabled ? 1 : 0),
                (unsigned long)ep.maxVelocity,
                (unsigned long)ep.maxAccel);
  }
  file.close();
  out.printf("CFG: wrote %s\n", kEndpointConfigPath);
  return true;
}

bool SdCardManager::loadAnimationConfig(const char *path, AppConfig &cfg, Stream &out) {
  if (!_ready) {
    return false;
  }
  FsFile file = _sd.open(path, O_RDONLY);
  if (!file) {
    return false;
  }

  bool inEndpoints = false;
  bool sawEndpoints = false;
  char line[160] = {};
  while (readLine(file, line, sizeof(line))) {
    char *cursor = line;
    while (*cursor == ' ' || *cursor == '\t') {
      cursor++;
    }
    if (*cursor == '\0') {
      continue;
    }
    if (isSectionLine(cursor, "endpoints")) {
      inEndpoints = true;
      sawEndpoints = true;
      continue;
    }
    if (isSectionLine(cursor, "sequence")) {
      inEndpoints = false;
      continue;
    }

    stripInlineComment(cursor);
    while (*cursor == ' ' || *cursor == '\t') {
      cursor++;
    }
    if (*cursor == '\0' || *cursor == '#') {
      continue;
    }
    if (!inEndpoints) {
      continue;
    }

    char *token = strtok(cursor, ",");
    char *tokens[8] = {};
    uint8_t count = 0;
    while (token && count < 8) {
      tokens[count++] = token;
      token = strtok(nullptr, ",");
    }
    if (count < 7) {
      out.printf("ANIM: skip line (need 7 fields): %s\n", line);
      continue;
    }

    uint32_t endpointId = 0;
    uint32_t port = 0;
    uint32_t address = 0;
    uint32_t motor = 0;
    uint32_t enabled = 0;
    uint32_t maxVelocity = 0;
    uint32_t maxAccel = 0;
    if (!parseUint(tokens[0], endpointId) ||
        !parseUint(tokens[1], port) ||
        !parseUint(tokens[2], address) ||
        !parseUint(tokens[3], motor) ||
        !parseUint(tokens[4], enabled)) {
      out.printf("ANIM: parse error: %s\n", line);
      continue;
    }
    if (count == 7) {
      if (!parseUint(tokens[5], maxVelocity) || !parseUint(tokens[6], maxAccel)) {
        out.printf("ANIM: max_velocity/max_accel parse error: %s\n", line);
        continue;
      }
    } else {
      if (!parseUint(tokens[6], maxVelocity) || !parseUint(tokens[7], maxAccel)) {
        out.printf("ANIM: max_velocity/max_accel parse error: %s\n", line);
        continue;
      }
    }
    if (endpointId == 0 || endpointId > MAX_ENDPOINTS) {
      out.printf("ANIM: invalid endpoint %lu\n", (unsigned long)endpointId);
      continue;
    }

    EndpointConfig &ep = cfg.endpoints[endpointId - 1];
    ep.serialPort = (port >= 1 && port <= 8) ? static_cast<uint8_t>(port) : ep.serialPort;
    ep.address = (address <= 0xFF) ? static_cast<uint8_t>(address) : ep.address;
    ep.motor = (motor == 1 || motor == 2) ? static_cast<uint8_t>(motor) : ep.motor;
    ep.enabled = (enabled != 0);
    ep.maxVelocity = maxVelocity;
    ep.maxAccel = maxAccel;
  }

  file.close();
  if (!sawEndpoints) {
    out.println("ANIM: no [endpoints] section found");
  }
  return sawEndpoints;
}

bool SdCardManager::saveAnimationConfig(const char *path, const AppConfig &cfg, Stream &out) {
  if (!_ready) {
    return false;
  }
  FsFile file = _sd.open(path, O_WRITE | O_CREAT | O_TRUNC);
  if (!file) {
    return false;
  }
  writeEndpointsSection(file, cfg);
  file.println();
  file.println("[sequence]");
  file.println("# time_ms,endpoint_id,position,velocity,accel,mode");
  file.close();
  out.printf("ANIM: wrote %s\n", path);
  return true;
}

bool SdCardManager::updateAnimationConfig(const char *path, const AppConfig &cfg, Stream &out) {
  if (!_ready) {
    return false;
  }
  if (!_sd.exists(path)) {
    return saveAnimationConfig(path, cfg, out);
  }

  FsFile input = _sd.open(path, O_RDONLY);
  if (!input) {
    return false;
  }
  const char *tmpPath = "/anim_tmp.csv";
  FsFile output = _sd.open(tmpPath, O_WRITE | O_CREAT | O_TRUNC);
  if (!output) {
    input.close();
    return false;
  }

  bool sawEndpoints = false;
  bool inEndpoints = false;
  bool insertedEndpoints = false;
  bool skipEndpoints = false;
  bool updated[MAX_ENDPOINTS] = {};
  char line[160] = {};
  while (readLine(input, line, sizeof(line))) {
    char original[160] = {};
    snprintf(original, sizeof(original), "%s", line);

    char *cursor = line;
    while (*cursor == ' ' || *cursor == '\t') {
      cursor++;
    }

    if (!sawEndpoints && isSectionLine(cursor, "sequence")) {
      writeEndpointsSection(output, cfg);
      output.println();
      insertedEndpoints = true;
      output.println(original);
      continue;
    }

    if (isSectionLine(cursor, "endpoints")) {
      sawEndpoints = true;
      if (insertedEndpoints) {
        inEndpoints = true;
        skipEndpoints = true;
      } else {
        inEndpoints = true;
        output.println(original);
      }
      continue;
    }

    if (isSectionLine(cursor, "sequence")) {
      if (inEndpoints) {
        if (skipEndpoints) {
          inEndpoints = false;
          skipEndpoints = false;
          output.println(original);
          continue;
        }
        bool addedAny = false;
        for (uint8_t i = 0; i < MAX_ENDPOINTS; i++) {
          if (!updated[i]) {
            if (!addedAny) {
              output.println("# auto-added endpoints");
              addedAny = true;
            }
            const EndpointConfig &ep = cfg.endpoints[i];
            output.printf("%u,%u,0x%02X,%u,%u,%lu,%lu\n",
                          (unsigned)(i + 1),
                          (unsigned)ep.serialPort,
                          (unsigned)ep.address,
                          (unsigned)ep.motor,
                          (unsigned)(ep.enabled ? 1 : 0),
                          (unsigned long)ep.maxVelocity,
                          (unsigned long)ep.maxAccel);
          }
        }
        inEndpoints = false;
      }
      output.println(original);
      continue;
    }

    if (inEndpoints) {
      if (skipEndpoints) {
        continue;
      }
      const char *trimmed = original;
      while (*trimmed == ' ' || *trimmed == '\t') {
        trimmed++;
      }
      if (*trimmed == '#') {
        if (strstr(trimmed, "endpoint_id") != nullptr) {
          output.println("# endpoint_id,serial_port,address,motor,enabled,max_velocity,max_accel");
          continue;
        }
      }
      char data[160] = {};
      snprintf(data, sizeof(data), "%s", original);
      char *comment = strchr(data, '#');
      if (comment) {
        *comment = '\0';
      }
      char *dataCursor = data;
      while (*dataCursor == ' ' || *dataCursor == '\t') {
        dataCursor++;
      }
      if (*dataCursor == '\0') {
        output.println(original);
        continue;
      }

      char prefix[32] = {};
      size_t prefixLen = 0;
      const char *origCursor = original;
      while (*origCursor == ' ' || *origCursor == '\t') {
        if (prefixLen + 1 < sizeof(prefix)) {
          prefix[prefixLen++] = *origCursor;
          prefix[prefixLen] = '\0';
        }
        origCursor++;
      }

      char *token = strtok(dataCursor, ",");
      if (token) {
        uint32_t endpointId = 0;
        if (parseUint(token, endpointId) && endpointId >= 1 && endpointId <= MAX_ENDPOINTS) {
          const EndpointConfig &ep = cfg.endpoints[endpointId - 1];
          output.print(prefix);
          output.printf("%u,%u,0x%02X,%u,%u,%lu,%lu",
                        (unsigned)endpointId,
                        (unsigned)ep.serialPort,
                        (unsigned)ep.address,
                        (unsigned)ep.motor,
                        (unsigned)(ep.enabled ? 1 : 0),
                        (unsigned long)ep.maxVelocity,
                        (unsigned long)ep.maxAccel);
          const char *commentOut = strchr(original, '#');
          if (commentOut) {
            output.print(" ");
            output.print(commentOut);
          }
          output.println();
          updated[endpointId - 1] = true;
          continue;
        }
      }

      output.println(original);
      continue;
    }

    output.println(original);
  }

  input.close();
  output.close();

  if (!sawEndpoints && !insertedEndpoints) {
    FsFile appendFile = _sd.open(tmpPath, O_WRITE | O_APPEND);
    if (appendFile) {
      appendFile.println();
      writeEndpointsSection(appendFile, cfg);
      appendFile.close();
    }
  }

  _sd.remove(path);
  _sd.rename(tmpPath, path);
  out.printf("ANIM: updated %s\n", path);
  return true;
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

bool SdCardManager::isSectionLine(const char *line, const char *section) const {
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
  file.println("[endpoints]");
  file.println("# endpoint_id,serial_port,address,motor,enabled,max_velocity,max_accel");
  for (uint8_t i = 0; i < MAX_ENDPOINTS; i++) {
    const EndpointConfig &ep = cfg.endpoints[i];
    file.printf("%u,%u,0x%02X,%u,%u,%lu,%lu\n",
                (unsigned)(i + 1),
                (unsigned)ep.serialPort,
                (unsigned)ep.address,
                (unsigned)ep.motor,
                (unsigned)(ep.enabled ? 1 : 0),
                (unsigned long)ep.maxVelocity,
                (unsigned long)ep.maxAccel);
  }
}

bool SdCardManager::openFile(const char *path, FsFile &file) {
  if (!_ready) {
    return false;
  }
  file = _sd.open(path, O_RDONLY);
  return static_cast<bool>(file);
}
