/* This file is gerated by pins-to-c.py script, do not modify. */
#include "pins.h"

static os_short pins_inputs_dip_switch_3_prm[]= {PIN_PULL_UP, 1};
static os_short pins_inputs_touch_sensor_prm[]= {PIN_TOUCH, 1};
static os_short pins_analog_inputs_potentiometer_prm[]= {PIN_SPEED, 3, PIN_DELAY, 11};
static os_short pins_pwm_servo_prm[]= {PIN_RESOLUTION, 12, PIN_FREQENCY, 50, PIN_INIT, 2048};
static os_short pins_pwm_dimmer_led_prm[]= {PIN_RESOLUTION, 12, PIN_FREQENCY, 5000, PIN_INIT, 0};

const pins_t pins =
{
  {{3, &pins.inputs.dip_switch_3},
    {"dip_switch_3", PIN_INPUT, 0, 34, pins_inputs_dip_switch_3_prm, sizeof(pins_inputs_dip_switch_3_prm)/sizeof(os_short), OS_NULL},
    {"dip_switch_4", PIN_INPUT, 0, 35, OS_NULL, 0, OS_NULL},
    {"touch_sensor", PIN_INPUT, 0, 4, pins_inputs_touch_sensor_prm, sizeof(pins_inputs_touch_sensor_prm)/sizeof(os_short), OS_NULL}
  },

  {{1, &pins.outputs.led_builtin},
    {"led_builtin", PIN_OUTPUT, 0, 2, OS_NULL, 0, OS_NULL}
  },

  {{1, &pins.analog_inputs.potentiometer},
    {"potentiometer", PIN_ANALOG_INPUT, 0, 25, pins_analog_inputs_potentiometer_prm, sizeof(pins_analog_inputs_potentiometer_prm)/sizeof(os_short), OS_NULL}
  },

  {{2, &pins.pwm.servo},
    {"servo", PIN_PWM, 0, 32, pins_pwm_servo_prm, sizeof(pins_pwm_servo_prm)/sizeof(os_short), OS_NULL},
    {"dimmer_led", PIN_PWM, 1, 33, pins_pwm_dimmer_led_prm, sizeof(pins_pwm_dimmer_led_prm)/sizeof(os_short), OS_NULL}
  }
};

static const PinGroupHdr *pins_group_list[] =
{
  &pins.inputs.hdr,
  &pins.outputs.hdr,
  &pins.analog_inputs.hdr,
  &pins.pwm.hdr
};

const IoPinsHdr pins_hdr = {pins_group_list, sizeof(pins_group_list)/sizeof(PinGroupHdr*)};
