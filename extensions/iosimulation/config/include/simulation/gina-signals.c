/* This file is gerated by signals-to-c.py script, do not modify. */
#include "iocom.h"
struct gina_t gina = 
{
  {
    {&ioboard_UP, 4, GINA_UP_MBLK_SZ, &gina.up.dip_switch_3},
    {30, 1, OS_BOOLEAN, 0, &ioboard_UP, &pins.inputs.dip_switch_3}, /* dip_switch_3 */
    {31, 1, OS_BOOLEAN, 0, &ioboard_UP, &pins.inputs.dip_switch_4}, /* dip_switch_4 */
    {32, 1, OS_BOOLEAN, 0, &ioboard_UP, &pins.inputs.touch_sensor}, /* touch_sensor */
    {33, 1, OS_USHORT, 0, &ioboard_UP, &pins.analog_inputs.potentiometer} /* potentiometer */
  },

  {
    {&ioboard_DOWN, 4, GINA_DOWN_MBLK_SZ, &gina.down.servo},
    {0, 1, OS_SHORT, 0, &ioboard_DOWN, &pins.pwm.servo}, /* servo */
    {3, 1, OS_SHORT, 0, &ioboard_DOWN, &pins.pwm.dimmer_led}, /* dimmer_led */
    {6, 1, OS_BOOLEAN, 0, &ioboard_DOWN, &pins.outputs.led_builtin}, /* led_builtin */
    {7, 8, OS_BOOLEAN, 0, &ioboard_DOWN} /* seven_segment */
  }
};

static const iocMblkSignalHdr *gina_mblk_list[] =
{
  &gina.up.hdr,
  &gina.down.hdr
};

const iocDeviceHdr gina_hdr = {gina_mblk_list, sizeof(gina_mblk_list)/sizeof(iocMblkSignalHdr*)};
