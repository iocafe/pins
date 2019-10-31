#include "pins.h"
static os_short jane_DIP_SWITCH_3_prm[]= {PIN_PULL_UP, 1};
const Pin jane_DIP_SWITCH_3 = {"DIP_SWITCH_3", PIN_INPUT, 0, 34, jane_DIP_SWITCH_3_prm, sizeof(jane_DIP_SWITCH_3_prm)/sizeof(os_short), OS_NULL, OS_NULL};
const Pin jane_DIP_SWITCH_4 = {"DIP_SWITCH_4", PIN_INPUT, 0, 35, OS_NULL, 0, OS_NULL, &jane_DIP_SWITCH_3};
static os_short jane_TOUCH_SENSOR_prm[]= {PIN_TOUCH, 1};
const Pin jane_TOUCH_SENSOR = {"TOUCH_SENSOR", PIN_INPUT, 0, 4, jane_TOUCH_SENSOR_prm, sizeof(jane_TOUCH_SENSOR_prm)/sizeof(os_short), OS_NULL, &jane_DIP_SWITCH_4};
const Pin *jane_pins = &jane_TOUCH_SENSOR;
const Pin jane_LED_BUILTIN = {"LED_BUILTIN", PIN_OUTPUT, 0, 2, OS_NULL, 0, OS_NULL, OS_NULL};
const Pin *jane_pins = &jane_LED_BUILTIN;
static os_short jane_POTENTIOMETER_prm[]= {PIN_SPEED, 3, PIN_DELAY, 11};
const Pin jane_POTENTIOMETER = {"POTENTIOMETER", PIN_ANALOG_INPUT, 0, 25, jane_POTENTIOMETER_prm, sizeof(jane_POTENTIOMETER_prm)/sizeof(os_short), OS_NULL, OS_NULL};
const Pin *jane_pins = &jane_POTENTIOMETER;
static os_short jane_SERVO_prm[]= {PIN_FREQENCY, 50, PIN_RESOLUTION, 12, PIN_INIT, 2048};
const Pin jane_SERVO = {"SERVO", PIN_PWM, 0, 32, jane_SERVO_prm, sizeof(jane_SERVO_prm)/sizeof(os_short), OS_NULL, OS_NULL};
static os_short jane_DIMMER_LED_prm[]= {PIN_FREQENCY, 5000, PIN_RESOLUTION, 12, PIN_INIT, 0};
const Pin jane_DIMMER_LED = {"DIMMER_LED", PIN_PWM, 1, 33, jane_DIMMER_LED_prm, sizeof(jane_DIMMER_LED_prm)/sizeof(os_short), OS_NULL, &jane_SERVO};
const Pin *jane_pins = &jane_DIMMER_LED;
