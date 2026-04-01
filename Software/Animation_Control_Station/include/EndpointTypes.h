#pragma once
#include <Arduino.h>
#include <cctype>
#include <cstring>
#include <cstdlib>

enum class EndpointType : uint8_t {
  AnimComOctal = 0,
  AnimComQuad = 1,
  Audio = 2
};

enum class EndpointItemType : uint8_t {
  None = 0,
  Motor = 1,
  Servo = 2,
  AudioTrack = 3
};

inline const char* endpointTypeName(EndpointType type) {
  switch (type) {
    case EndpointType::AnimComOctal: return "ANIMCOM_OCTAL";
    case EndpointType::AnimComQuad: return "ANIMCOM_QUAD";
    case EndpointType::Audio: return "AUDIO";
    default: return "UNKNOWN";
  }
}

inline const char* endpointTypeShortName(EndpointType type) {
  switch (type) {
    case EndpointType::AnimComOctal: return "ACO";
    case EndpointType::AnimComQuad: return "ACQ";
    case EndpointType::Audio: return "AUD";
    default: return "UNK";
  }
}

inline const char* endpointItemTypeName(EndpointItemType type) {
  switch (type) {
    case EndpointItemType::Motor: return "MOTOR";
    case EndpointItemType::Servo: return "SERVO";
    case EndpointItemType::AudioTrack: return "AUDIO";
    case EndpointItemType::None:
    default:
      return "NONE";
  }
}

inline const char* endpointItemTypeShortName(EndpointItemType type) {
  switch (type) {
    case EndpointItemType::Motor: return "M";
    case EndpointItemType::Servo: return "S";
    case EndpointItemType::AudioTrack: return "A";
    case EndpointItemType::None:
    default:
      return "-";
  }
}

inline bool isSupportedEndpointType(EndpointType type) {
  return (type == EndpointType::AnimComOctal ||
          type == EndpointType::AnimComQuad ||
          type == EndpointType::Audio);
}

inline uint8_t endpointTypeMaxItems(EndpointType type) {
  switch (type) {
    case EndpointType::AnimComOctal: return 8;
    case EndpointType::AnimComQuad: return 4;
    case EndpointType::Audio: return 8;
    default: return 0;
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
    if (numeric >= 0 && numeric <= static_cast<long>(EndpointType::Audio)) {
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

  if (strcmp(buf, "ANIMCOM_OCTAL") == 0 || strcmp(buf, "ACO") == 0) {
    type = EndpointType::AnimComOctal;
    return true;
  }
  if (strcmp(buf, "ANIMCOM_QUAD") == 0 || strcmp(buf, "ACQ") == 0) {
    type = EndpointType::AnimComQuad;
    return true;
  }
  if (strcmp(buf, "AUDIO") == 0 || strcmp(buf, "AUD") == 0) {
    type = EndpointType::Audio;
    return true;
  }
  return false;
}

inline bool parseEndpointItemType(const char *text, EndpointItemType &type) {
  if (!text) {
    return false;
  }
  while (*text == ' ' || *text == '\t') {
    text++;
  }
  if (*text == '\0') {
    return false;
  }

  char buf[24] = {};
  size_t len = 0;
  while (*text != '\0' && *text != ' ' && *text != '\t' && *text != ',' && len < sizeof(buf) - 1) {
    buf[len++] = static_cast<char>(toupper(static_cast<unsigned char>(*text)));
    text++;
  }
  buf[len] = '\0';

  if (strcmp(buf, "NONE") == 0 || strcmp(buf, "-") == 0) {
    type = EndpointItemType::None;
    return true;
  }
  if (strcmp(buf, "MOTOR") == 0 || strcmp(buf, "M") == 0) {
    type = EndpointItemType::Motor;
    return true;
  }
  if (strcmp(buf, "SERVO") == 0 || strcmp(buf, "S") == 0) {
    type = EndpointItemType::Servo;
    return true;
  }
  if (strcmp(buf, "AUDIO") == 0 || strcmp(buf, "TRACK") == 0 || strcmp(buf, "A") == 0) {
    type = EndpointItemType::AudioTrack;
    return true;
  }
  return false;
}
