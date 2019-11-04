/* This file is gerated by pins-to-c.py script, do not modify. */
OSAL_C_HEADER_BEGINS

typedef struct
{
  struct
  {
    PinGroupHdr hdr;
    Pin dip_switch_3;
    Pin dip_switch_4;
    Pin touch_sensor;
  }
  inputs;

  struct
  {
    PinGroupHdr hdr;
    Pin led_builtin;
    Pin A;
    Pin B;
    Pin C;
    Pin D;
    Pin E;
    Pin F;
    Pin G;
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

extern const IoPinsHdr pins_hdr;
extern const pins_t pins;
extern const Pin *pins_segment7_group;

OSAL_C_HEADER_ENDS
