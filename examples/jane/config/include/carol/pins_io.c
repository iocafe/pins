/* This file is gerated by pins_to_c.py script, do not modify. */
#include "pins.h"

/* Parameters for inputs */
static os_ushort pins_inputs_gazerbeam_prm[]= {PIN_RV, PIN_RV, PIN_INTERRUPT_ENABLED, 1};
PINS_INTCONF_STRUCT(pin_gazerbeam_intconf)
static os_ushort pins_inputs_dip_switch_3_prm[]= {PIN_RV, PIN_RV, PIN_PULL_UP, 1};
static os_ushort pins_inputs_dip_switch_4_prm[]= {PIN_RV, PIN_RV};
static os_ushort pins_inputs_touch_sensor_prm[]= {PIN_RV, PIN_RV, PIN_TOUCH, 1};

/* Parameters for outputs */
static os_ushort pins_outputs_led_builtin_prm[]= {PIN_RV, PIN_RV};

/* Parameters for analog_inputs */
static os_ushort pins_analog_inputs_potentiometer_prm[]= {PIN_RV, PIN_RV, PIN_SPEED, 3, PIN_MAX, 4095};

/* Parameters for pwm */
static os_ushort pins_pwm_servo_prm[]= {PIN_RV, PIN_RV, PIN_FREQENCY, 50, PIN_RESOLUTION, 12, PIN_INIT, 2048, PIN_MAX, 4095};
static os_ushort pins_pwm_dimmer_led_prm[]= {PIN_RV, PIN_RV, PIN_FREQENCY, 5000, PIN_RESOLUTION, 12, PIN_INIT, 0, PIN_MAX, 4095};

/* Parameters for uart */
static os_ushort pins_uart_uart2_prm[]= {PIN_RV, PIN_RV, PIN_RX, 16, PIN_TX, 17, PIN_SPEED, 96};

/* JANE IO configuration structure */
OS_FLASH_MEM pins_t pins =
{
  {{4, &pins.inputs.gazerbeam}, /* inputs */
    {PIN_INPUT, 0, 39, pins_inputs_gazerbeam_prm, sizeof(pins_inputs_gazerbeam_prm)/sizeof(os_ushort), OS_NULL, OS_NULL PINS_INTCONF_PTR(pin_gazerbeam_intconf)}, /* gazerbeam */
    {PIN_INPUT, 0, 34, pins_inputs_dip_switch_3_prm, sizeof(pins_inputs_dip_switch_3_prm)/sizeof(os_ushort), OS_NULL, OS_NULL PINS_INTCONF_NULL}, /* dip_switch_3 */
    {PIN_INPUT, 0, 35, pins_inputs_dip_switch_4_prm, sizeof(pins_inputs_dip_switch_4_prm)/sizeof(os_ushort), OS_NULL, OS_NULL PINS_INTCONF_NULL}, /* dip_switch_4 */
    {PIN_INPUT, 0, 36, pins_inputs_touch_sensor_prm, sizeof(pins_inputs_touch_sensor_prm)/sizeof(os_ushort), OS_NULL, OS_NULL PINS_INTCONF_NULL} /* touch_sensor */
  },

  {{1, &pins.outputs.led_builtin}, /* outputs */
    {PIN_OUTPUT, 0, 2, pins_outputs_led_builtin_prm, sizeof(pins_outputs_led_builtin_prm)/sizeof(os_ushort), OS_NULL, OS_NULL PINS_INTCONF_NULL} /* led_builtin */
  },

  {{1, &pins.analog_inputs.potentiometer}, /* analog_inputs */
    {PIN_ANALOG_INPUT, 0, 26, pins_analog_inputs_potentiometer_prm, sizeof(pins_analog_inputs_potentiometer_prm)/sizeof(os_ushort), OS_NULL, OS_NULL PINS_INTCONF_NULL} /* potentiometer */
  },

  {{2, &pins.pwm.servo}, /* pwm */
    {PIN_PWM, 0, 22, pins_pwm_servo_prm, sizeof(pins_pwm_servo_prm)/sizeof(os_ushort), OS_NULL, OS_NULL PINS_INTCONF_NULL}, /* servo */
    {PIN_PWM, 1, 27, pins_pwm_dimmer_led_prm, sizeof(pins_pwm_dimmer_led_prm)/sizeof(os_ushort), OS_NULL, OS_NULL PINS_INTCONF_NULL} /* dimmer_led */
  },

  {{1, &pins.uart.uart2}, /* uart */
    {PIN_UART, 0, 2, pins_uart_uart2_prm, sizeof(pins_uart_uart2_prm)/sizeof(os_ushort), OS_NULL, OS_NULL PINS_INTCONF_NULL} /* uart2 */
  },
};

/* List of pin type groups */
static OS_FLASH_MEM PinGroupHdr * OS_FLASH_MEM pins_group_list[] =
{
  &pins.inputs.hdr,
  &pins.outputs.hdr,
  &pins.analog_inputs.hdr,
  &pins.pwm.hdr,
  &pins.uart.hdr
};

/* JANE IO configuration top header structure */
OS_FLASH_MEM IoPinsHdr pins_hdr = {pins_group_list, sizeof(pins_group_list)/sizeof(PinGroupHdr*)};
