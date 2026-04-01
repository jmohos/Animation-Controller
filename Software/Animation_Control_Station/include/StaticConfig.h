#pragma once
#include <Arduino.h>
#include "ControllerTypes.h"
#include "EndpointTypes.h"

static constexpr uint8_t kStaticEndpointMaxItems = 8;

struct StaticEndpointDef {
  uint8_t stationId;
  EndpointType type;
  uint8_t itemCount;
  EndpointItemType itemTypes[kStaticEndpointMaxItems];
};

struct StaticItemDef {
  const char *name;
  uint8_t stationId;
  uint8_t channelIndex;
  ManualParameter parameter;
  int32_t minValue;
  int32_t maxValue;
  int32_t defaultValue;
  int32_t step;
};

static constexpr uint8_t kStaticEndpointCount = 8;
static constexpr uint8_t kStaticItemCount = 13;
static constexpr uint8_t kRunPatternMin = 1;
static constexpr uint8_t kRunPatternMax = 3;
static constexpr uint8_t kRunPatternDefault = 1;
static constexpr uint8_t kManualGridColumns = 2;

extern const StaticEndpointDef kStaticEndpoints[kStaticEndpointCount];
extern const StaticItemDef kStaticItems[kStaticItemCount];
