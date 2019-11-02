/* This file is gerated by pins-to-c.py script, do not modify. */
#include "pins.h"
static os_short io_DIP_SWITCH_3_prm[]= {PIN_PULL_UP, 1};
const Pin io_DIP_SWITCH_3 = {"DIP_SWITCH_3", PIN_INPUT, 0, 34, io_DIP_SWITCH_3_prm, sizeof(io_DIP_SWITCH_3_prm)/sizeof(os_short), OS_NULL, OS_NULL};
const Pin io_DIP_SWITCH_4 = {"DIP_SWITCH_4", PIN_INPUT, 0, 35, OS_NULL, 0, OS_NULL, &io_DIP_SWITCH_3};
static os_short io_TOUCH_SENSOR_prm[]= {PIN_TOUCH, 1};
const Pin io_TOUCH_SENSOR = {"TOUCH_SENSOR", PIN_INPUT, 0, 4, io_TOUCH_SENSOR_prm, sizeof(io_TOUCH_SENSOR_prm)/sizeof(os_short), OS_NULL, &io_DIP_SWITCH_4};
const Pin io_LED_BUILTIN = {"LED_BUILTIN", PIN_OUTPUT, 0, 2, OS_NULL, 0, OS_NULL, &io_TOUCH_SENSOR};
static os_short io_POTENTIOMETER_prm[]= {PIN_SPEED, 3, PIN_DELAY, 11};
const Pin io_POTENTIOMETER = {"POTENTIOMETER", PIN_ANALOG_INPUT, 0, 25, io_POTENTIOMETER_prm, sizeof(io_POTENTIOMETER_prm)/sizeof(os_short), OS_NULL, &io_LED_BUILTIN};
static os_short io_SERVO_prm[]= {PIN_INIT, 2048, PIN_FREQENCY, 50, PIN_RESOLUTION, 12};
const Pin io_SERVO = {"SERVO", PIN_PWM, 0, 32, io_SERVO_prm, sizeof(io_SERVO_prm)/sizeof(os_short), OS_NULL, &io_POTENTIOMETER};
static os_short io_DIMMER_LED_prm[]= {PIN_INIT, 0, PIN_FREQENCY, 5000, PIN_RESOLUTION, 12};
const Pin io_DIMMER_LED = {"DIMMER_LED", PIN_PWM, 1, 33, io_DIMMER_LED_prm, sizeof(io_DIMMER_LED_prm)/sizeof(os_short), OS_NULL, &io_SERVO};
const Pin *io_pins = &io_DIMMER_LED;
