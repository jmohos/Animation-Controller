#include "SdCard.h"
#include "ConfigStore.h"
#include "BoardPins.h"
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
    char *tokens[16] = {};
    uint8_t count = 0;
    while (token && count < 16) {
      tokens[count++] = token;
      token = strtok(nullptr, ",");
    }
    // Support both 12-field (old) and 16-field (new) format
    if (count < 12) {
      out.printf("CFG: skip line (need 12 or 16 fields): %s\n", line);
      continue;
    }

    uint32_t endpointId = 0;
    EndpointType type = EndpointType::RoboClaw;
    uint32_t address = 0;
    uint32_t enabled = 0;
    int32_t posMin = 0;
    int32_t posMax = 0;
    uint32_t velMin = 0;
    uint32_t velMax = 0;
    uint32_t accMin = 0;
    uint32_t accMax = 0;
    uint32_t port = 0;
    uint32_t motor = 0;
    if (!parseUint(tokens[0], endpointId) ||
        !parseEndpointType(tokens[1], type) ||
        !parseUint(tokens[2], address) ||
        !parseUint(tokens[3], enabled) ||
        !parseInt(tokens[4], posMin) ||
        !parseInt(tokens[5], posMax) ||
        !parseUint(tokens[6], velMin) ||
        !parseUint(tokens[7], velMax) ||
        !parseUint(tokens[8], accMin) ||
        !parseUint(tokens[9], accMax) ||
        !parseUint(tokens[10], port) ||
        !parseUint(tokens[11], motor)) {
      out.printf("CFG: parse error: %s\n", line);
      continue;
    }

    // Parse new fields (12-15) if present in 16-field format
    uint32_t pulsesPerRev = 0;
    int32_t homeOffset = 0;
    uint32_t homeDir = 0;
    uint32_t hasLimit = 0;

    if (count >= 13) {
      if (!parseUint(tokens[12], pulsesPerRev)) {
        out.printf("CFG: parse error (pulses_per_rev): %s\n", line);
        continue;
      }
    }
    if (count >= 14) {
      parseInt(tokens[13], homeOffset);  // Optional, ignore parse errors
    }
    if (count >= 15) {
      parseUint(tokens[14], homeDir);    // Optional, ignore parse errors
    }
    if (count >= 16) {
      parseUint(tokens[15], hasLimit);   // Optional, ignore parse errors
    }

    if (endpointId == 0 || endpointId > MAX_ENDPOINTS) {
      out.printf("CFG: invalid endpoint %lu\n", (unsigned long)endpointId);
      continue;
    }
    const bool usesCan = (type == EndpointType::MksServo ||
                          type == EndpointType::RevFrcCan ||
                          type == EndpointType::JoeServoCan);
    if (usesCan && enabled != 0) {
      if (port != 0) {
        out.printf("CFG: CAN port must be 0: %s\n", line);
        continue;
      }
      if (type == EndpointType::MksServo && address > 0x7FFu) {
        out.printf("CFG: MKS CAN ID must be 0-0x7FF: %s\n", line);
        continue;
      }
    } else if (enabled != 0) {
      if (port < 1 || port > RS422_PORT_COUNT) {
        out.printf("CFG: serial port must be 1-%u: %s\n", (unsigned)RS422_PORT_COUNT, line);
        continue;
      }
    }
    if (type == EndpointType::RoboClaw && enabled != 0) {
      if (port < 1 || port > RS422_PORT_COUNT) {
        out.printf("CFG: RoboClaw port must be 1-%u: %s\n", (unsigned)RS422_PORT_COUNT, line);
        continue;
      }
      if (motor < 1 || motor > 2) {
        out.printf("CFG: RoboClaw motor must be 1-2: %s\n", line);
        continue;
      }
    }

    EndpointConfig &ep = cfg.endpoints[endpointId - 1];
    ep.type = type;
    ep.address = address;
    ep.enabled = (enabled != 0);
    ep.positionMin = posMin;
    ep.positionMax = posMax;
    ep.velocityMin = velMin;
    ep.velocityMax = velMax;
    ep.accelMin = accMin;
    ep.accelMax = accMax;
    ep.serialPort = (port <= RS422_PORT_COUNT) ? static_cast<uint8_t>(port) : ep.serialPort;
    ep.motor = (motor <= 2) ? static_cast<uint8_t>(motor) : ep.motor;

    // Assign new calibration and homing fields
    ep.pulsesPerRevolution = pulsesPerRev;
    ep.homeOffset = homeOffset;
    ep.homeDirection = (homeDir != 0) ? 1 : 0;
    ep.hasLimitSwitch = (hasLimit != 0) ? 1 : 0;
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
  file.println("# endpoint_id,type,address,enabled,position_min,position_max,velocity_min,velocity_max,accel_min,accel_max,serial_port,motor,pulses_per_rev,home_offset,home_direction,has_limit_switch");
  file.println("# Note: position_min/max in degrees, velocity in deg/s, accel in deg/s² when pulses_per_rev > 0");
  for (uint8_t i = 0; i < MAX_ENDPOINTS; i++) {
    const EndpointConfig &ep = cfg.endpoints[i];
    file.printf("%u,%s,0x%08lX,%u,%ld,%ld,%lu,%lu,%lu,%lu,%u,%u,%lu,%ld,%u,%u\n",
                (unsigned)(i + 1),
                endpointTypeName(ep.type),
                (unsigned long)ep.address,
                (unsigned)(ep.enabled ? 1 : 0),
                (long)ep.positionMin,
                (long)ep.positionMax,
                (unsigned long)ep.velocityMin,
                (unsigned long)ep.velocityMax,
                (unsigned long)ep.accelMin,
                (unsigned long)ep.accelMax,
                (unsigned)ep.serialPort,
                (unsigned)ep.motor,
                (unsigned long)ep.pulsesPerRevolution,
                (long)ep.homeOffset,
                (unsigned)ep.homeDirection,
                (unsigned)ep.hasLimitSwitch);
  }
  file.close();
  out.printf("CFG: wrote %s\n", kEndpointConfigPath);
  return true;
}

bool SdCardManager::loadAnimationConfig(const char *path, AppConfig &cfg, Stream &out) {
  (void)cfg;
  if (!_ready) {
    return false;
  }
  FsFile file = _sd.open(path, O_RDONLY);
  if (!file) {
    return false;
  }
  file.close();
  out.println("ANIM: endpoints moved to endpoints.csv");
  return false;
}

bool SdCardManager::saveAnimationConfig(const char *path, const AppConfig &cfg, Stream &out) {
  (void)cfg;
  if (!_ready) {
    return false;
  }
  FsFile file = _sd.open(path, O_WRITE | O_CREAT | O_TRUNC);
  if (!file) {
    return false;
  }
  file.println("[sequence]");
  file.println("# time_ms,endpoint_id,position,velocity,accel,mode");
  file.close();
  out.printf("ANIM: wrote %s\n", path);
  return true;
}

bool SdCardManager::saveDefaultAnimation(const char *path, Stream &out) {
  if (!_ready) {
    return false;
  }
  FsFile file = _sd.open(path, O_WRITE | O_CREAT | O_TRUNC);
  if (!file) {
    return false;
  }
  file.println("[sequence]");
  file.println("# time_ms,endpoint_id,position,velocity,accel,mode");
  file.println("0,1,0,800,250,pos");
  file.println("0,2,0,800,250,pos");
  file.println("0,3,0,800,250,pos");
  file.println("0,4,0,800,250,pos");
  file.println("2000,1,1000,800,250,pos");
  file.println("2000,2,1000,800,250,pos");
  file.println("2000,3,1000,800,250,pos");
  file.println("2000,4,1000,800,250,pos");
  file.println("4000,1,0,800,250,pos");
  file.println("4000,2,0,800,250,pos");
  file.println("4000,3,0,800,250,pos");
  file.println("4000,4,0,800,250,pos");
  file.println("6000,1,-1000,800,250,pos");
  file.println("6000,2,-1000,800,250,pos");
  file.println("6000,3,-1000,800,250,pos");
  file.println("6000,4,-1000,800,250,pos");
  file.println("8000,1,0,800,250,pos");
  file.println("8000,2,0,800,250,pos");
  file.println("8000,3,0,800,250,pos");
  file.println("8000,4,0,800,250,pos");
  file.close();
  out.printf("ANIM: wrote default %s\n", path);
  return true;
}

bool SdCardManager::updateAnimationConfig(const char *path, const AppConfig &cfg, Stream &out) {
  (void)cfg;
  if (!_ready) {
    return false;
  }
  if (_sd.exists(path)) {
    out.printf("ANIM: exists %s\n", path);
    return true;
  }
  return saveAnimationConfig(path, cfg, out);
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
  file.println("# endpoint_id,type,address,enabled,position_min,position_max,velocity_min,velocity_max,accel_min,accel_max,serial_port,motor,pulses_per_rev,home_offset,home_direction,has_limit_switch");
  file.println("# Note: position_min/max in degrees, velocity in deg/s, accel in deg/s² when pulses_per_rev > 0");
  for (uint8_t i = 0; i < MAX_ENDPOINTS; i++) {
    const EndpointConfig &ep = cfg.endpoints[i];
    file.printf("%u,%s,0x%08lX,%u,%ld,%ld,%lu,%lu,%lu,%lu,%u,%u,%lu,%ld,%u,%u\n",
                (unsigned)(i + 1),
                endpointTypeName(ep.type),
                (unsigned long)ep.address,
                (unsigned)(ep.enabled ? 1 : 0),
                (long)ep.positionMin,
                (long)ep.positionMax,
                (unsigned long)ep.velocityMin,
                (unsigned long)ep.velocityMax,
                (unsigned long)ep.accelMin,
                (unsigned long)ep.accelMax,
                (unsigned)ep.serialPort,
                (unsigned)ep.motor,
                (unsigned long)ep.pulsesPerRevolution,
                (long)ep.homeOffset,
                (unsigned)ep.homeDirection,
                (unsigned)ep.hasLimitSwitch);
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
