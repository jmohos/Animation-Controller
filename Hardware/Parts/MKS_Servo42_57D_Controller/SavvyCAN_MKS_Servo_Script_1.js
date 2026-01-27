// MKS SERVO42D/57D CAN Decoder Script for SavvyCAN
// Decodes messages based on command byte (first data byte)

function setup() {
    host.log("MKS SERVO Decoder Started");
    // Register to receive all CAN frames from bus 0
    // Mask 0x0 means accept all IDs
    can.setFilter(0x0, 0x0, 0);
    host.log("Listening for MKS SERVO messages on Bus 0");
}

function gotCANFrame(bus, id, len, data) {
    // Only process frames from MKS SERVO motors (IDs 0x01-0xFF)
    if (id < 1 || id > 255) return;
    
    if (len < 2) return; // Need at least command + CRC
    
    var motorID = id;
    var cmd = data[0];
    var output = "Motor 0x" + motorID.toString(16).toUpperCase() + ": ";
    
    // Decode based on command byte
    switch(cmd) {
        // Read-only parameters (0x30-0x42)
        case 0x30: // Read encoder value
            if (len >= 8) {
                var carry = getInt32(data, 1);
                var value = getUint16(data, 5);
                output += "ENCODER - Carry: " + carry + ", Value: " + value + " (0x" + value.toString(16) + ")";
                output += ", Angle: " + (value * 360.0 / 16384.0).toFixed(2) + "°";
            }
            break;
            
        case 0x31: // Read cumulative encoder
            if (len >= 8) {
                var cumValue = getInt48(data, 1);
                output += "CUMULATIVE ENCODER: " + cumValue;
                output += ", Rotations: " + (cumValue / 16384.0).toFixed(3);
            }
            break;
            
        case 0x32: // Read speed
            if (len >= 4) {
                var speed = getInt16(data, 1);
                output += "SPEED: " + speed + " RPM";
                if (speed > 0) output += " (CCW)";
                else if (speed < 0) output += " (CW)";
            }
            break;
            
        case 0x33: // Read pulse count
            if (len >= 6) {
                var pulses = getInt32(data, 1);
                output += "PULSE COUNT: " + pulses;
            }
            break;
            
        case 0x34: // Read IO port status
            if (len >= 3) {
                var status = data[1];
                output += "IO STATUS - IN_1: " + ((status & 0x01) ? "1" : "0");
                output += ", IN_2: " + ((status & 0x02) ? "1" : "0");
                output += ", OUT_1: " + ((status & 0x10) ? "1" : "0");
                output += ", OUT_2: " + ((status & 0x20) ? "1" : "0");
            }
            break;
            
        case 0x35: // Read raw encoder
            if (len >= 8) {
                var rawValue = getInt48(data, 1);
                output += "RAW ENCODER: " + rawValue;
            }
            break;
            
        case 0x39: // Read angle error
            if (len >= 6) {
                var error = getInt32(data, 1);
                var errorDeg = error * 360.0 / 51200.0;
                output += "ANGLE ERROR: " + error + " (" + errorDeg.toFixed(2) + "°)";
            }
            break;
            
        case 0x3A: // Read enable status
            if (len >= 3) {
                var enabled = data[1];
                output += "ENABLE STATUS: " + (enabled ? "ENABLED" : "DISABLED");
            }
            break;
            
        case 0x3B: // Read home status
            if (len >= 4) {
                var status1 = data[1];
                var status2 = data[2];
                var statusText = ["In Progress", "Success", "Failed"];
                output += "HOME STATUS - Single Turn: " + statusText[status1];
                output += ", Home: " + statusText[status2];
            }
            break;
            
        case 0x3E: // Read stall status
            if (len >= 3) {
                var stalled = data[1];
                output += "STALL STATUS: " + (stalled ? "STALLED" : "OK");
            }
            break;
            
        case 0x40: // Read version
            if (len >= 5) {
                var cal = (data[1] >> 4) & 0x0F;
                var hwVer = data[1] & 0x0F;
                var fwVer = (data[2] << 16) | (data[3] << 8) | data[4];
                var hwNames = ["?", "S42D_485", "S42D_CAN", "S57D_485", "S57D_CAN", 
                               "S28D_485", "S28D_CAN", "S35D_485", "S35D_CAN"];
                output += "VERSION - HW: " + hwNames[hwVer] + ", FW: 0x" + fwVer.toString(16);
                output += ", Calibrated: " + (cal ? "YES" : "NO");
            }
            break;
            
        // Configuration commands (0x80-0x9F)
        case 0x80: // Calibrate encoder
            if (len >= 3) {
                var status = data[1];
                var statusText = ["Calibrating...", "Success", "Failed"];
                output += "CALIBRATION: " + statusText[status];
            }
            break;
            
        case 0x82: // Set work mode
            if (len >= 3) {
                var mode = data[1];
                var modes = ["CR_OPEN", "CR_CLOSE", "CR_vFOC", "SR_OPEN", "SR_CLOSE", "SR_vFOC"];
                output += "SET MODE: " + (mode < modes.length ? modes[mode] : "Unknown(" + mode + ")");
            }
            break;
            
        case 0x83: // Set current
            if (len >= 4) {
                var current = getUint16(data, 1);
                output += "SET CURRENT: " + current + " mA";
                if (len >= 5 && data[3] === 0x00) {
                    output += " (not saved)";
                }
            }
            break;
            
        case 0x84: // Set subdivisions
            if (len >= 3) {
                var microstep = data[1];
                output += "SET SUBDIVISIONS: " + microstep;
            }
            break;
            
        case 0x85: // Set EN level
            if (len >= 3) {
                var level = data[1];
                var levels = ["Active Low", "Active High", "Always Active"];
                output += "SET EN LEVEL: " + levels[level];
            }
            break;
            
        case 0x86: // Set direction
            if (len >= 3) {
                var dir = data[1];
                output += "SET DIRECTION: " + (dir ? "CCW" : "CW");
            }
            break;
            
        case 0x8B: // Set CAN ID
            if (len >= 4) {
                var newID = getUint16(data, 1);
                output += "SET CAN ID: 0x" + newID.toString(16).toUpperCase();
            }
            break;
            
        // Motion control commands (0xF1-0xFF)
        case 0xF1: // Read motor status
            if (len >= 3) {
                var status = data[1];
                var statusText = ["Failed", "Stopped", "Accel", "Decel", "Full Speed", 
                                  "Homing", "Calibrating"];
                output += "MOTOR STATUS: " + statusText[status];
            }
            break;
            
        case 0xF3: // Set enable
            if (len >= 3) {
                var enable = data[1];
                output += "SET ENABLE: " + (enable ? "ENABLED" : "DISABLED");
            }
            break;
            
        case 0xF4: // Position mode - relative coordinate
            if (len >= 8) {
                var speed = getUint16(data, 1);
                var acc = data[3];
                var coord = getInt24(data, 4);
                output += "POS REL COORD - Speed: " + speed + " RPM, Acc: " + acc;
                output += ", Coord: " + coord + " (" + (coord/16384.0).toFixed(3) + " rot)";
            } else if (len == 3) {
                // Response message
                var status = data[1];
                var statusText = ["FAILED", "STARTING", "COMPLETE", "LIMIT STOPPED", "?", "SYNC RECEIVED"];
                output += "POS REL COORD Response: " + statusText[status];
            }
            break;
            
        case 0xF5: // Position mode - absolute coordinate
            if (len >= 8) {
                var speed = getUint16(data, 1);
                var acc = data[3];
                var coord = getInt24(data, 4);
                output += "POS ABS COORD - Speed: " + speed + " RPM, Acc: " + acc;
                output += ", Coord: " + coord + " (" + (coord/16384.0).toFixed(3) + " rot)";
            } else if (len == 3) {
                // Response message
                var status = data[1];
                var statusText = ["FAILED", "STARTING", "COMPLETE", "LIMIT STOPPED", "?", "SYNC RECEIVED"];
                output += "POS ABS COORD Response: " + statusText[status];
            }
            break;
            
        case 0xF6: // Speed control mode
            if (len >= 5) {
                var dirSpeed = getUint16(data, 1);
                var dir = (dirSpeed & 0x8000) ? "CW" : "CCW";
                var speed = dirSpeed & 0x0FFF;
                var acc = data[3];
                output += "SPEED MODE - Dir: " + dir + ", Speed: " + speed + " RPM, Acc: " + acc;
                if (len >= 8) {
                    var runtime = getUint24(data, 4);
                    output += ", Runtime: " + (runtime * 10) + " ms";
                }
            } else if (len == 3) {
                // Response message
                var status = data[1];
                var statusText = ["FAILED", "STARTING", "COMPLETE", "?", "?", "SYNC RECEIVED"];
                output += "SPEED MODE Response: " + statusText[status];
            }
            break;
            
        case 0xF7: // Emergency stop
            if (len >= 3) {
                var status = data[1];
                output += "EMERGENCY STOP: " + (status ? "Success" : "Failed");
            }
            break;
            
        case 0xFD: // Position mode - relative pulse
            if (len >= 8) {
                var dirSpeed = getUint16(data, 1);
                var dir = (dirSpeed & 0x8000) ? "CW" : "CCW";
                var speed = dirSpeed & 0x0FFF;
                var acc = data[3];
                var pulses = getUint24(data, 4);
                output += "POS REL PULSE - Dir: " + dir + ", Speed: " + speed + " RPM";
                output += ", Acc: " + acc + ", Pulses: " + pulses;
            } else if (len == 3) {
                // Response message
                var status = data[1];
                var statusText = ["FAILED", "STARTING", "COMPLETE", "LIMIT STOPPED", "?", "SYNC RECEIVED"];
                output += "POS REL PULSE Response: " + statusText[status];
            }
            break;
            
        case 0xFE: // Position mode - absolute pulse
            if (len >= 8) {
                var speed = getUint16(data, 1);
                var acc = data[3];
                var pulses = getInt24(data, 4);
                output += "POS ABS PULSE - Speed: " + speed + " RPM, Acc: " + acc;
                output += ", Pulses: " + pulses;
            } else if (len == 3) {
                // Response message
                var status = data[1];
                var statusText = ["FAILED", "STARTING", "COMPLETE", "LIMIT STOPPED", "?", "SYNC RECEIVED"];
                output += "POS ABS PULSE Response: " + statusText[status];
            }
            break;
            
        default:
            output += "CMD 0x" + cmd.toString(16).toUpperCase();
            break;
    }
    
    // Add CRC
    var crc = data[len - 1];
    output += " [CRC: 0x" + crc.toString(16).toUpperCase() + "]";
    
    // Output the decoded message
    host.log(output);
}

// Helper functions to extract multi-byte values
function getUint16(data, offset) {
    return (data[offset] << 8) | data[offset + 1];
}

function getInt16(data, offset) {
    var val = getUint16(data, offset);
    if (val & 0x8000) val = val - 0x10000;
    return val;
}

function getUint24(data, offset) {
    return (data[offset] << 16) | (data[offset + 1] << 8) | data[offset + 2];
}

function getInt24(data, offset) {
    var val = getUint24(data, offset);
    if (val & 0x800000) val = val - 0x1000000;
    return val;
}

function getInt32(data, offset) {
    var val = (data[offset] << 24) | (data[offset + 1] << 16) | 
              (data[offset + 2] << 8) | data[offset + 3];
    return val;
}

function getInt48(data, offset) {
    // JavaScript can handle up to 53-bit integers safely
    var val = 0;
    for (var i = 0; i < 6; i++) {
        val = (val * 256) + data[offset + i];
    }
    // Handle sign extension for 48-bit signed
    if (val >= 0x800000000000) {
        val = val - 0x1000000000000;
    }
    return val;
}