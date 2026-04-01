# Animation Control Station
The purpose of this controller is to manage a network of animation endpoints.  Supported endpoints are from a list of compatible animation controller endpoints.  These include an octal motor controller, a quad motor controller and an audio playback node.  

## Animation Endpoint Configuration
The endpoint mapping for the controller shall be captured as a static configuration table.  In the future the option to edit the table can be added to the UI while in STOP mode.  For now assume the following configuration:

| Station # | Item # | Name           | Parameter     | Range     |
| --------- | ------ | -------------- | ------------- |---------- |
| 1         | 0      | Left Pinwheel  | Speed Percent | -100, 100 |
| 2         | 0      | Right Pinwheel | Speed Percent | -100, 100 |
| 3         | 0      | Generator      | Speed deg/sec | 0, 720    |
| 4         | 0      | Crane Pivot    | Position deg  | 0, 180    |
| 4         | 1      | Crane Winch    | Position deg  | 0, 360    |
| 5         | 0      | Flower Pivot   | Position deg  | 0, 180    |
| 5         | 1      | Light Pivot    | Position deg  | 0, 360    |
| 6         | 0      | Robot Head     | Position deg  | 0, 180    |
| 6         | 1      | Robot Arms     | Position deg  | 0, 270    |
| 7         | 0      | Meter          | Position deg  | 0, 180    |
| 8         | 0      | Tesla Ring     | Position deg  | 0, 360    |
| 8         | 1      | Lion Tilt      | Position deg  | 0, 90     |
| 8         | 2      | Lion Hair      | Position deg  | 0, 45     |

## Animation Control Communications Protocol
The animation controller shall repeatedly send messages to all of the animation stations using a shared RS485 interface.

### Mode Command Message
The controller shall send a periodic Mode Command message to all stations with the current operating mode selected by the operator.  This includes "STOP", "RUN" and "MANUAL".

When sending STOP, no additional parameters are necessary.
When sending RUN, the animation pattern number and speed scalar is also sent.

### Manual Control Message
When operating in MANUAL_RUN mode, a Manual Command Message is filled in and sent to the selected station number with either Speed Percent, Speed Deg/Sec, or Position Deg populated.

## User Interface Components
The animation controller consists of:
* Color LCD display
* 5 button menu keyboard (up, down, left, right, OK)
* Rotary quadrature encoder.
* A lightable red button.
* A lightable yellow button.
* A lightable green button.

The red, green and yellow buttons are the primary mode selectors.  The rotary encoder adjusts the highlighted value that can be adjusted in that mode (speed, etc.) The 5-way keyboard is used to adjust items on the screen like selecting manual parameters to adjust.

## Operating Mode
The operator will have the ability to select the following operating modes for the network:

* STOP - All animated features come to a stop immediately.
* RUN - All animated features cycle through their preprogrammed animation patterns.  The animation pattern number is sent to select between animation patterns 0-x.  
* MANUAL_HOLD - All animation features are commanded to stop and the operator is expected to select the item to manually adjust.
* MANUAL_RUN - The item selected for manual control is commanded with the value of the rotary encoder knob.  The type of value is dependent on the selection made on the screen during MANUAL_HOLD.

A press of the red button will immediately enter STOP mode and light only the red button.
A press of the green button will immediately enter the RUN mode and light only the green button.
A press of the yellow button will toggle between MANUAL_HOLD and MANUAL_RUN.  If transitioning from STOP or RUN then we transition to MANUAL_HOLD where the operator can pick the item to adjust.  Then a subsequent press of the yellow button will transition to MANUAL_RUN.  The yellow button light will light up when in MANUAL_RUN mode. 

## User Experience
### Screen Layout
The display screen will have an indicator in the upper left corner of the current operating mode.  This will include "STOPPED", "RUNNING X", "MANUAL HOLD" or "MANUAL RUN".

The bottom of the screen will be a bar with text indicating what the buttons will do when pressed.  The background color of the bottom indicator matches the placement of the physical buttons.  The lower left third is the stop button in red, the center third is the manual buttin in yellow, the lower right third is the run button in green.

### STOP Mode User Experience
In stop mode, the screen does not show any additional information as the top left mode indicator and the light stop button will indicate the stopped state.  The menu and rotary encoder do not change anything.

### RUN Mode User Experience
In run mode, the screen will show two items in the middle of the screen:

1. The current animation speed selection.  By default the animation speed values is 100%.  The rotary knob can be adjusted to change this value from 0 to 200%.  This speed scalar is sent to all the endpoints as the RUN mode speed scalar to collectively speed or slow down the animation.
2. A selector for the animation pattern to run.  This will appear as "ANIM #: xx" where the xx is a numeric field that is adjustable with the menu up and down keys.  Hitting up or down will alternate through a list of values like 1, 2, 3.  If the selection is not the current one then the field blinks.  Once the operator hits enter then the menu selection becomes the animation number that is sent in the RUN command to all of the endpoints.  Hint text near the bottom of the screen will inform the operators of what the Up/Down/OK and knob do.

### Manual Mode User Experience
When the operator hits the yellow button, it will toggle into MANUAL_HOLD and then to MANAUAL_RUN modes. The screen will present a list of all of the animation items.

While in MANUAL_HOLD mode, The Up, Down, Left, Right keys will move a cursor selecting one of these items with either a blink pattern or backlight of the text.

While in MANUAL_RUN mode, the selected item during MANUAL_HOLD is presented as a field along with its adjustment variable.  This can be either speed percent, speed in deg/sec, position in deg or an audio trigger.  The rotary encoder input then effects this variable by incrementing or decrementing the value up to its limits.

An example for MANUAL_HOLD is to present the following as selectable items:
"LEFT PINWHEELS"
"RIGHT PINWHEELS"
"GENERATOR"
"CRANE PIVOT"
"CRANE WINCH"
"FLOWER PIVOT"
"LIGHT PIVOT"
"ROBOT HEAD"
"ROBOT ARMS"
"METER"
"TESLA RING"
"LION TILT"
"LION HAIR"

The system should remember the last index between activations of manual mode to help navigate quickly.

Here are some examples of MANUAL_RUN:
"LEFT PINWHEELS: -45%"
or
"GENERATOR: 180 deg/sec"
or
"METER: 35 deg"



