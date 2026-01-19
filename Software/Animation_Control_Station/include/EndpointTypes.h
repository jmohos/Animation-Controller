#pragma once
#include <Arduino.h>
#include <cctype>
#include <cstring>
#include <cstdlib>

enum class EndpointType : uint8_t {
  RoboClaw = 0,
  MksServo = 1,
  RevFrcCan = 2,
  JoeServoSerial = 3,
  JoeServoCan = 4
};

inline const char* endpointTypeName(EndpointType type) {
  switch (type) {
    case EndpointType::RoboClaw: return "ROBOCLAW";
    case EndpointType::MksServo: return "MKS_SERVO";
    case EndpointType::RevFrcCan: return "REV_FRC_CAN";
    case EndpointType::JoeServoSerial: return "JOE_SERVO_SERIAL";
    case EndpointType::JoeServoCan: return "JOE_SERVO_CAN";
    default: return "UNKNOWN";
  }
}

inline const char* endpointTypeShortName(EndpointType type) {
  switch (type) {
    case EndpointType::RoboClaw: return "RC";
    case EndpointType::MksServo: return "MKS";
    case EndpointType::RevFrcCan: return "REV";
    case EndpointType::JoeServoSerial: return "JS";
    case EndpointType::JoeServoCan: return "JC";
    default: return "UNK";
  }
}

inline bool parseEndpointType(const char *text, EndpointType &type) {
  if (!text) {
    return false;
  }
  while (*text == ' ' || *text == '\t') {
    text++;
  }
  if (*text == '\0') {
    return false;
  }

  char *end = nullptr;
  long numeric = strtol(text, &end, 10);
  if (end != text && (*end == '\0' || *end == ',')) {
    if (numeric >= 0 && numeric <= static_cast<long>(EndpointType::JoeServoCan)) {
      type = static_cast<EndpointType>(numeric);
      return true;
    }
  }

  char buf[32] = {};
  size_t len = 0;
  while (*text != '\0' && *text != ' ' && *text != '\t' && *text != ',' && len < sizeof(buf) - 1) {
    buf[len++] = static_cast<char>(toupper(static_cast<unsigned char>(*text)));
    text++;
  }
  buf[len] = '\0';

  if (strcmp(buf, "ROBOCLAW") == 0 || strcmp(buf, "RC") == 0) {
    type = EndpointType::RoboClaw;
    return true;
  }
  if (strcmp(buf, "MKS_SERVO") == 0 || strcmp(buf, "MKS") == 0) {
    type = EndpointType::MksServo;
    return true;
  }
  if (strcmp(buf, "REV_FRC_CAN") == 0 || strcmp(buf, "REV") == 0 || strcmp(buf, "FRC_CAN") == 0) {
    type = EndpointType::RevFrcCan;
    return true;
  }
  if (strcmp(buf, "JOE_SERVO_SERIAL") == 0 || strcmp(buf, "JOE_SERIAL") == 0 || strcmp(buf, "JS") == 0) {
    type = EndpointType::JoeServoSerial;
    return true;
  }
  if (strcmp(buf, "JOE_SERVO_CAN") == 0 || strcmp(buf, "JOE_CAN") == 0 || strcmp(buf, "JC") == 0) {
    type = EndpointType::JoeServoCan;
    return true;
  }
  return false;
}
