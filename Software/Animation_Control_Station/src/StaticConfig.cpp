#include "StaticConfig.h"

const StaticEndpointDef kStaticEndpoints[kStaticEndpointCount] = {
  {1, EndpointType::AnimComOctal, 1, {EndpointItemType::Motor, EndpointItemType::None, EndpointItemType::None, EndpointItemType::None, EndpointItemType::None, EndpointItemType::None, EndpointItemType::None, EndpointItemType::None}},
  {2, EndpointType::AnimComOctal, 1, {EndpointItemType::Motor, EndpointItemType::None, EndpointItemType::None, EndpointItemType::None, EndpointItemType::None, EndpointItemType::None, EndpointItemType::None, EndpointItemType::None}},
  {3, EndpointType::AnimComQuad, 1, {EndpointItemType::Motor, EndpointItemType::None, EndpointItemType::None, EndpointItemType::None, EndpointItemType::None, EndpointItemType::None, EndpointItemType::None, EndpointItemType::None}},
  {4, EndpointType::AnimComQuad, 2, {EndpointItemType::Motor, EndpointItemType::Motor, EndpointItemType::None, EndpointItemType::None, EndpointItemType::None, EndpointItemType::None, EndpointItemType::None, EndpointItemType::None}},
  {5, EndpointType::AnimComQuad, 2, {EndpointItemType::Motor, EndpointItemType::Motor, EndpointItemType::None, EndpointItemType::None, EndpointItemType::None, EndpointItemType::None, EndpointItemType::None, EndpointItemType::None}},
  {6, EndpointType::AnimComQuad, 2, {EndpointItemType::Motor, EndpointItemType::Motor, EndpointItemType::None, EndpointItemType::None, EndpointItemType::None, EndpointItemType::None, EndpointItemType::None, EndpointItemType::None}},
  {7, EndpointType::AnimComQuad, 1, {EndpointItemType::Motor, EndpointItemType::None, EndpointItemType::None, EndpointItemType::None, EndpointItemType::None, EndpointItemType::None, EndpointItemType::None, EndpointItemType::None}},
  {8, EndpointType::AnimComQuad, 3, {EndpointItemType::Motor, EndpointItemType::Motor, EndpointItemType::Servo, EndpointItemType::None, EndpointItemType::None, EndpointItemType::None, EndpointItemType::None, EndpointItemType::None}}
};

const StaticItemDef kStaticItems[kStaticItemCount] = {
  {"Left Pinwheel", 1, 0, ManualParameter::SpeedPercent, -100, 100, 0, 1},
  {"Right Pinwheel", 2, 0, ManualParameter::SpeedPercent, -100, 100, 0, 1},
  {"Generator", 3, 0, ManualParameter::VelocityDegPerSec, 0, 720, 0, 5},
  {"Crane Pivot", 4, 0, ManualParameter::PositionDeg, 0, 180, 0, 1},
  {"Crane Winch", 4, 1, ManualParameter::PositionDeg, 0, 360, 0, 1},
  {"Flower Pivot", 5, 0, ManualParameter::PositionDeg, 0, 180, 0, 1},
  {"Light Pivot", 5, 1, ManualParameter::PositionDeg, 0, 360, 0, 1},
  {"Robot Head", 6, 0, ManualParameter::PositionDeg, 0, 180, 0, 1},
  {"Robot Arms", 6, 1, ManualParameter::PositionDeg, 0, 270, 0, 1},
  {"Meter", 7, 0, ManualParameter::PositionDeg, 0, 180, 0, 1},
  {"Tesla Ring", 8, 0, ManualParameter::PositionDeg, 0, 360, 0, 1},
  {"Lion Tilt", 8, 1, ManualParameter::PositionDeg, 0, 90, 0, 1},
  {"Lion Hair", 8, 2, ManualParameter::PositionDeg, 0, 45, 0, 1}
};
