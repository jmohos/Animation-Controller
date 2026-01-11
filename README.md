# Animation-Controller
Robotic Animation Control Project

## VS Code Workspace
Use the workspace file when building, uploading, or monitoring the serial port so VS Code opens the full project with PlatformIO settings.

Prerequisites:
- Visual Studio Code
- PlatformIO IDE extension for VS Code (installs toolchains and dependencies on first build)
- Teensy 4.1 USB drivers for your OS (Windows: Teensyduino installer includes the driver)
- A known-good USB data cable for upload and serial monitoring

1. Open VS Code.
2. File -> Open Workspace from File...
3. Select `2025_Animation_Controller.code-workspace` in the repo root.
4. Use the PlatformIO toolbar to Build, Upload, and Monitor the serial port.
5. First build: PlatformIO will download toolchains and libraries. Select the Teensy serial port and use 115200 baud for the monitor.

## Display
The control station software currently targets the ILI9341 240x320 display only.
Other display drivers and build options have been removed.

## Functional System Description
To faciliate the orchestration of an animated robotic system, an animation controller assembly is needed.  This controller takes on the responsibility of monitoring and controlling all participating show elements in a synchronized manner.  A typical parade float can consist of a dozen animated features that each require managed motion across potentially multiple dimensions.  By having a single control node manage all of the animated features, all of the elements can be synchronized into a cohesive show where coordinated motion can add to the quality of the performance. This project covers the design of such an animation controller.

The key high level requirements include:
* A single control assembly that can operate on 12VDC power input.
* Has a user interface for the local animation operator to monitor the overall animation, control the selected animation process, manually control elements and edit the animation sequence.
* Can control a series of robotic endpoints using up to 8 full-duplex serial ports.
* Robotic endpoints can be either motion, light or sound controllers.
* The motion control endpoints will include the BasicMicro Roboclaw controllers which can control either 1 or 2 motors via a single serial interface.
* Future options for motion control endpoints will include options to manage stepper motors, servos, solenoids and hydraulic valves.
* Future options for light control will include endpoints to manage DMX-based light controllers.
* Future options for sound control will include endpoints that can receive commands to play audio tracks.
* Animation sequences shall be stored in a portable animation sequence file that contains the time-based steps for each animation endpoint.
* Animation sequences are stored on the animation controller SD card.

## Animation Sequence File (CSV)
The controller reads a human-editable CSV file from the SD card. The file defines a single looping animation with a maximum duration of 5 minutes (300000 ms).

Schema (CSV columns, in order):
1. `time_ms` - Integer milliseconds from 0 to 300000.
2. `endpoint_id` - 1-based endpoint index (maps to a separate endpoint configuration table).
3. `position` - Target position in encoder counts.
4. `velocity` - Max velocity in encoder counts per second.
5. `accel` - Max acceleration in encoder counts per second squared.

Example:
```
# time_ms,endpoint_id,position,velocity,accel
0,1,0,0,0
0,2,0,0,0
500,1,1200,2000,1000
500,2,800,2000,1000
```

### Endpoint Configuration Table
Endpoints map to a serial port and RoboClaw address in a separate config table (stored on SD or in EEPROM). The animation file references `endpoint_id` only.

Default mapping (RoboClaw 2x30A, dual-motor):
- Endpoints 1-2 -> Serial 1 (endpoint 1 = M1, endpoint 2 = M2)
- Endpoints 3-4 -> Serial 2 (endpoint 3 = M1, endpoint 4 = M2)
- Endpoints 5-6 -> Serial 3 (endpoint 5 = M1, endpoint 6 = M2)
- Endpoints 7-8 -> Serial 4 (endpoint 7 = M1, endpoint 8 = M2)
- Endpoints 9-10 -> Serial 5 (endpoint 9 = M1, endpoint 10 = M2)
- Endpoints 11-12 -> Serial 6 (endpoint 11 = M1, endpoint 12 = M2)
- Endpoints 13-14 -> Serial 7 (endpoint 13 = M1, endpoint 14 = M2)
- Endpoints 15-16 -> Serial 8 (endpoint 15 = M1, endpoint 16 = M2)

Notes:
- Lines starting with `#` are comments and ignored.
- Rows may be in any order; playback is sorted by `time_ms`.
- `velocity` and `accel` are required on every row for consistency.
- The animation loops at the largest `time_ms` found, capped at 300000 ms.


### Hardware - Overall System
The animator controller is the heart of a complete animation deployment used for shows like parade floats and general animatronic performances.  Such shows require multiple robotic actuators to work independently or in concert to perform synchronized motion, light and sound sequences.

A deployed system would typically consist of:
* Robotic power system to provide electrical power to the animation controller, motion controllers, lights and sound systems.
* A robotic animation controller to manage the entire show process.  This controller would contain a scripted set of animations to orchestrate across all of the animated elements.
* A set of robotic endpoints that the animation controller manages.  Each of these endpoints communicates with the animation controller via a serial link.  Endpoints report their status and receive control commands from the animation controller.  A typical command is to move an actuator to a given position at a given max speed and given max acceleration.  It can also command things like light patterns and sound triggers.
* The animation controller has a user interface where the animation operator can select from recorded animation patters, enable play/stop of the patterns or edit and manually control the endpoints.

### Hardware - Animation Controller
The animation controller assembly consists of the following:
* A Teensy 4.1 microcontroller board to manage the entire system.
* A 12VDC power input port with DC:DC converters to 5VDC and 3.3VDC.
* The power conversion stage has reverse polarity protection and has transient suppression above 18v.
* An LCD display connected to the Teensy 4.1 primary SPI.  It is a ILI9341 based display with 240x320 pixel resolution.
* A SX1509 IO expander board from Sparkfun connected to the Teensy 4.1 to provide 8 pins of button inputs and 8 pins of LED outputs.  By using this I2C device we free up the other pins of the Teensy 4.1.
* Two 10K Ohm potentiometers attached to Teensy 4.1 analog inputs.  These physical knobs allow the operator to input desired speed and acceleration parameters. The inputs are scaled from 0.0 to 100.0 with deadbands at the bottom and top to ensure we can reach the limits with the potentiometer.
* A rotary encoder jog wheel connected to the Teensy 4.1 to provide a physical positioning input by the operator.  It uses a two input quadrature encoder to count the incremental steps either forward or backwards.
* A set of buttons to provide menu navigation for the menu system displayed on the screen.  The five buttons include up, down, left, right and Ok.  This is implemented using a 5 way tactile button located next to the display.
* A set of three context sensitive buttons with LED backlights.  These buttons are located below the display to align with a set of hot-key displays at the bottom of the screen guiding you on what the three buttons currently do when pressed.  The buttons are Red (left), Yellow (center), and Green (right).
* The backlights for the Red, Yellow and Green buttons are controlled by the SX1509 IO expander outputs.  The output of the SX1509 outputs 8-15 are wired to an ULN2803 driver chip that switches 12V selectively to the LED outputs.
* The LED outputs can be commanded to be off, on or blink with a selectable on and off time to create unique patterns.
* The 8 hardware serial ports of the Teensy 4.1 are wired to a set of 8 connectors with VCC, Ground, RX and TX each.  These connectors mate with a set of 8 RS422, full-duplex serial converters that wire to each of the 8 remote actuator stations.
* An SD card slot on the Teensy 4.1 will be used for storage of the animation sequences.
* The Teensy 4.1 FLASH EEPROM will be used to store configuration settings like the number animation endpoints, their modes and their enables.
* The animation controller shall operate in a series of operating states that can include:
  * BOOT - System initializing and restoring EEPROM config and last configured SD card animation sequence into RAM.
  * MANUAL - The UI is focused on manually manipulating a single animation endpoint at a time.  The Red button is used to toggle between play and pause.  The yellow button, while held pressed, is used to select the endpoint number using the jog wheel.  The green button is used to trigger a move command to the endpoint.  The jog wheel, when yellow is not held, is used to adjust target position.  The speed and accel knobs are used to set the max speed and acceleration factor for the move.
  * AUTO - The UI is focused on displaying what time index in the animation sequence we are at, what the motion activity is.  If the red button is toggled, it will switch between play and pause mode.  While paused, the job wheel can be used to manually cycle through the animation sequence time and thus advance each channel in concert to allow for forward and backwards slow motion playback.
  * EDIT - TBD, operators will be able to edit new animation sequences for each endpoint using the speed, accel and jog wheel along with the red, yellow and green buttons to cue the next time marker and endpoint position.
* Animation playback shall use methods to smooth the playback transitions between animation timestamps to prevent the motion from being jerky.
