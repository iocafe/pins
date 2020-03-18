/* This file is gerated by pins-to-c.py script, do not modify. */
OSAL_C_HEADER_BEGINS

/* JANE IO configuration structure */
typedef struct
{
  struct
  {
    PinGroupHdr hdr;
    Pin dip_switch_3;
    Pin dip_switch_4;
    Pin gazerbeam;
    Pin touch_sensor;
  }
  inputs;

  struct
  {
    PinGroupHdr hdr;
    Pin led_builtin;
  }
  outputs;

  struct
  {
    PinGroupHdr hdr;
    Pin potentiometer;
  }
  analog_inputs;

  struct
  {
    PinGroupHdr hdr;
    Pin servo;
    Pin dimmer_led;
  }
  pwm;
}
pins_t;

/* JANE IO configuration top header structure */
extern const IoPinsHdr pins_hdr;

/* Global JANE IO configuration structure */
extern const pins_t pins;

/* Name defines for pins and application pin groups (use ifdef to check if HW has pin) */
#define PINS_INPUTS_DIP_SWITCH_3 "dip_switch_3"
#define PINS_INPUTS_DIP_SWITCH_4 "dip_switch_4"
#define PINS_INPUTS_GAZERBEAM "gazerbeam"
#define PINS_INPUTS_TOUCH_SENSOR "touch_sensor"
#define PINS_OUTPUTS_LED_BUILTIN "led_builtin"
#define PINS_ANALOG_INPUTS_POTENTIOMETER "potentiometer"
#define PINS_PWM_SERVO "servo"
#define PINS_PWM_DIMMER_LED "dimmer_led"

OSAL_C_HEADER_ENDS
