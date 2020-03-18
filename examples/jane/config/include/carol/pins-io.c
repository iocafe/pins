/* This file is gerated by pins-to-c.py script, do not modify. */
#include "pins.h"

/* Parameters for inputs */
static os_short pins_inputs_dip_switch_3_prm[]= {PIN_RV, PIN_RV, PIN_PULL_UP, 1};
static os_short pins_inputs_dip_switch_4_prm[]= {PIN_RV, PIN_RV};
static os_short pins_inputs_gazerbeam_prm[]= {PIN_RV, PIN_RV, PIN_INTERRUPT, 1};
PINS_INTCONF_STRUCT(pin_gazerbeam_intconf)
static os_short pins_inputs_touch_sensor_prm[]= {PIN_RV, PIN_RV, PIN_TOUCH, 1};

/* Parameters for outputs */
static os_short pins_outputs_led_builtin_prm[]= {PIN_RV, PIN_RV};

/* Parameters for analog_inputs */
static os_short pins_analog_inputs_potentiometer_prm[]= {PIN_RV, PIN_RV, PIN_SPEED, 3, PIN_DELAY, 11};

/* Parameters for pwm */
static os_short pins_pwm_servo_prm[]= {PIN_RV, PIN_RV, PIN_RESOLUTION, 12, PIN_INIT, 2048, PIN_FREQENCY, 50};
static os_short pins_pwm_dimmer_led_prm[]= {PIN_RV, PIN_RV, PIN_RESOLUTION, 12, PIN_INIT, 0, PIN_FREQENCY, 5000};

/* JANE IO configuration structure */
const pins_t pins =
{
  {{4, &pins.inputs.dip_switch_3}, /* inputs */
    {PIN_INPUT, 0, 34, pins_inputs_dip_switch_3_prm, sizeof(pins_inputs_dip_switch_3_prm)/sizeof(os_short), OS_NULL, OS_NULL PINS_INTCONF_NULL}, /* dip_switch_3 */
    {PIN_INPUT, 0, 35, pins_inputs_dip_switch_4_prm, sizeof(pins_inputs_dip_switch_4_prm)/sizeof(os_short), OS_NULL, OS_NULL PINS_INTCONF_NULL}, /* dip_switch_4 */
    {PIN_INPUT, 0, 36, pins_inputs_gazerbeam_prm, sizeof(pins_inputs_gazerbeam_prm)/sizeof(os_short), OS_NULL, OS_NULL PINS_INTCONF_PTR(pin_gazerbeam_intconf)}, /* gazerbeam */
    {PIN_INPUT, 0, 4, pins_inputs_touch_sensor_prm, sizeof(pins_inputs_touch_sensor_prm)/sizeof(os_short), OS_NULL, OS_NULL PINS_INTCONF_NULL} /* touch_sensor */
  },

  {{1, &pins.outputs.led_builtin}, /* outputs */
    {PIN_OUTPUT, 0, 2, pins_outputs_led_builtin_prm, sizeof(pins_outputs_led_builtin_prm)/sizeof(os_short), OS_NULL, OS_NULL PINS_INTCONF_NULL} /* led_builtin */
  },

  {{1, &pins.analog_inputs.potentiometer}, /* analog_inputs */
    {PIN_ANALOG_INPUT, 0, 25, pins_analog_inputs_potentiometer_prm, sizeof(pins_analog_inputs_potentiometer_prm)/sizeof(os_short), OS_NULL, OS_NULL PINS_INTCONF_NULL} /* potentiometer */
  },

  {{2, &pins.pwm.servo}, /* pwm */
    {PIN_PWM, 0, 32, pins_pwm_servo_prm, sizeof(pins_pwm_servo_prm)/sizeof(os_short), OS_NULL, OS_NULL PINS_INTCONF_NULL}, /* servo */
    {PIN_PWM, 1, 33, pins_pwm_dimmer_led_prm, sizeof(pins_pwm_dimmer_led_prm)/sizeof(os_short), OS_NULL, OS_NULL PINS_INTCONF_NULL} /* dimmer_led */
  }
};

/* List of pin type groups */
static const PinGroupHdr *pins_group_list[] =
{
  &pins.inputs.hdr,
  &pins.outputs.hdr,
  &pins.analog_inputs.hdr,
  &pins.pwm.hdr
};

/* JANE IO configuration top header structure */
const IoPinsHdr pins_hdr = {pins_group_list, sizeof(pins_group_list)/sizeof(PinGroupHdr*)};
