#include "UnitConversion.h"
#include <Arduino.h>

int32_t UnitConverter::degreesToPulses(float degrees, const EndpointConfig &ep) {
  // Passthrough if engineering units not enabled
  if (!usesEngineeringUnits(ep)) {
    return static_cast<int32_t>(degrees);
  }

  // Get pulses per revolution (use default if not configured)
  uint32_t ppr = (ep.pulsesPerRevolution > 0) ? ep.pulsesPerRevolution : getDefaultPulsesPerRev(ep.type);

  // Convert: degrees * (pulses_per_rev / 360.0)
  float pulses = degrees * (static_cast<float>(ppr) / 360.0f);
  return static_cast<int32_t>(pulses);
}

uint32_t UnitConverter::degPerSecToDeviceVelocity(float degPerSec, const EndpointConfig &ep) {
  // Passthrough if engineering units not enabled
  if (!usesEngineeringUnits(ep)) {
    return static_cast<uint32_t>(degPerSec);
  }

  if (ep.type == EndpointType::MksServo) {
    // MKS Servo uses RPM: Convert deg/s → RPM
    // RPM = (deg/s * 60) / 360 = deg/s / 6
    float rpm = (degPerSec * 60.0f) / 360.0f;
    if (rpm < 0.0f) rpm = 0.0f;  // Velocity must be non-negative
    if (rpm > static_cast<float>(MksServo::MAX_VELOCITY_RPM)) {
      rpm = static_cast<float>(MksServo::MAX_VELOCITY_RPM);
    }
    return static_cast<uint32_t>(rpm);
  } else {
    // RoboClaw uses counts/sec: Convert deg/s → counts/sec
    uint32_t ppr = (ep.pulsesPerRevolution > 0) ? ep.pulsesPerRevolution : getDefaultPulsesPerRev(ep.type);
    float countsPerSec = degPerSec * (static_cast<float>(ppr) / 360.0f);
    if (countsPerSec < 0.0f) countsPerSec = 0.0f;
    return static_cast<uint32_t>(countsPerSec);
  }
}

uint32_t UnitConverter::degPerSec2ToDeviceAccel(float degPerSec2, const EndpointConfig &ep) {
  // Passthrough if engineering units not enabled
  if (!usesEngineeringUnits(ep)) {
    return static_cast<uint32_t>(degPerSec2);
  }

  if (ep.type == EndpointType::MksServo) {
    // MKS Servo uses 0-255 scale (non-linear)
    // Linear approximation: Map 0-100 deg/s² → 0-255
    float scale = (degPerSec2 / 100.0f) * 255.0f;
    if (scale < 0.0f) scale = 0.0f;
    if (scale > 255.0f) scale = 255.0f;
    return static_cast<uint32_t>(scale);
  } else {
    // RoboClaw uses counts/sec²: Convert deg/s² → counts/sec²
    uint32_t ppr = (ep.pulsesPerRevolution > 0) ? ep.pulsesPerRevolution : getDefaultPulsesPerRev(ep.type);
    float countsPerSec2 = degPerSec2 * (static_cast<float>(ppr) / 360.0f);
    if (countsPerSec2 < 0.0f) countsPerSec2 = 0.0f;
    return static_cast<uint32_t>(countsPerSec2);
  }
}

float UnitConverter::pulsesToDegrees(int32_t pulses, const EndpointConfig &ep) {
  // Passthrough if engineering units not enabled
  if (!usesEngineeringUnits(ep)) {
    return static_cast<float>(pulses);
  }

  // Get pulses per revolution
  uint32_t ppr = (ep.pulsesPerRevolution > 0) ? ep.pulsesPerRevolution : getDefaultPulsesPerRev(ep.type);
  if (ppr == 0) return static_cast<float>(pulses);  // Avoid division by zero

  // Convert: pulses * (360.0 / pulses_per_rev)
  float degrees = static_cast<float>(pulses) * (360.0f / static_cast<float>(ppr));
  return degrees;
}

float UnitConverter::deviceVelocityToDegPerSec(uint32_t velocity, const EndpointConfig &ep) {
  // Passthrough if engineering units not enabled
  if (!usesEngineeringUnits(ep)) {
    return static_cast<float>(velocity);
  }

  if (ep.type == EndpointType::MksServo) {
    // MKS Servo velocity is in RPM: Convert RPM → deg/s
    // deg/s = (RPM * 360) / 60 = RPM * 6
    float degPerSec = static_cast<float>(velocity) * 6.0f;
    return degPerSec;
  } else {
    // RoboClaw velocity is in counts/sec: Convert counts/sec → deg/s
    uint32_t ppr = (ep.pulsesPerRevolution > 0) ? ep.pulsesPerRevolution : getDefaultPulsesPerRev(ep.type);
    if (ppr == 0) return static_cast<float>(velocity);
    float degPerSec = static_cast<float>(velocity) * (360.0f / static_cast<float>(ppr));
    return degPerSec;
  }
}

bool UnitConverter::usesEngineeringUnits(const EndpointConfig &ep) {
  return (ep.pulsesPerRevolution != 0);
}

uint32_t UnitConverter::getDefaultPulsesPerRev(EndpointType type) {
  switch (type) {
    case EndpointType::MksServo:
      return MksServo::DEFAULT_PULSES_PER_REV;  // 262144 (16 microstep)
    case EndpointType::RoboClaw:
      return 4096;  // Common encoder resolution
    case EndpointType::RevFrcCan:
      return 4096;  // FRC standard
    case EndpointType::JoeServoSerial:
    case EndpointType::JoeServoCan:
      return 4096;  // Assume standard resolution
    default:
      return 4096;
  }
}
