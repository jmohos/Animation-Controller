#pragma once
#include "ConfigStore.h"
#include "Utils.h"

/**
 * UnitConverter provides conversion between engineering units and device-specific units.
 *
 * Engineering Units:
 * - Position: degrees (multi-revolution support)
 * - Velocity: degrees per second
 * - Acceleration: degrees per second squared
 *
 * Device Units:
 * - MKS Servo: pulses (position), RPM (velocity), 0-255 scale (accel)
 * - RoboClaw: encoder counts (position), counts/sec (velocity), counts/sec² (accel)
 *
 * Conversion is enabled when EndpointConfig.pulsesPerRevolution > 0.
 * When pulsesPerRevolution = 0, values pass through unchanged (device units mode).
 */
class UnitConverter {
public:
  /**
   * Convert engineering units (degrees) to device units (pulses/counts).
   * @param degrees Position in degrees
   * @param ep Endpoint configuration with calibration data
   * @return Position in device-specific units (pulses or counts)
   */
  static int32_t degreesToPulses(float degrees, const EndpointConfig &ep);

  /**
   * Convert engineering units (deg/s) to device-specific velocity units.
   * @param degPerSec Velocity in degrees per second
   * @param ep Endpoint configuration with calibration data
   * @return Velocity in device units (RPM for MKS, counts/sec for RoboClaw)
   */
  static uint32_t degPerSecToDeviceVelocity(float degPerSec, const EndpointConfig &ep);

  /**
   * Convert engineering units (deg/s²) to device-specific acceleration units.
   * @param degPerSec2 Acceleration in degrees per second squared
   * @param ep Endpoint configuration with calibration data
   * @return Acceleration in device units (0-255 for MKS, counts/sec² for RoboClaw)
   */
  static uint32_t degPerSec2ToDeviceAccel(float degPerSec2, const EndpointConfig &ep);

  /**
   * Convert device units (pulses/counts) to engineering units (degrees).
   * @param pulses Position in device-specific units
   * @param ep Endpoint configuration with calibration data
   * @return Position in degrees
   */
  static float pulsesToDegrees(int32_t pulses, const EndpointConfig &ep);

  /**
   * Convert device-specific velocity units to engineering units (deg/s).
   * @param velocity Velocity in device units (RPM or counts/sec)
   * @param ep Endpoint configuration with calibration data
   * @return Velocity in degrees per second
   */
  static float deviceVelocityToDegPerSec(uint32_t velocity, const EndpointConfig &ep);

  /**
   * Check if an endpoint is configured for engineering units.
   * @param ep Endpoint configuration
   * @return true if pulsesPerRevolution > 0 (engineering units enabled)
   */
  static bool usesEngineeringUnits(const EndpointConfig &ep);

  /**
   * Get default pulses per revolution for an endpoint type.
   * @param type Endpoint type
   * @return Default pulses per revolution
   */
  static uint32_t getDefaultPulsesPerRev(EndpointType type);
};
