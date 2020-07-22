/**

  @file    pigpio/pins_pigpio_pwm.c
  @brief   pigpio pulse width modulation.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    21.7.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "pins.h"
#include <pigpio.h>

/**
****************************************************************************************************

  @brief Configure a pin as PWM.
  @anchor pin_pwm_setup

  The pin_pwm_setup() function...

  range: 25-40000, means that resolution_bits can be from 5 to 15.

  @param   pin Pointer to the pin structure.
  @return  None.

****************************************************************************************************
*/
void pin_pwm_setup(
    const Pin *pin)
{
    unsigned resolution_bits, range;

    resolution_bits = (unsigned)pin_get_prm(pin, PIN_RESOLUTION);
    if (!resolution_bits) resolution_bits = 12;
    range = (1 << resolution_bits) - 1;

    if (gpioSetPWMrange((unsigned)pin->addr, range) < 0) {
        osal_debug_error_int("gpioSetPWMrange(x,v) failed. Range 5 - 15. x=", pin->addr);
    }
    else if (gpioSetPWMfrequency((unsigned)pin->addr, (unsigned)pin_get_frequency(pin, 50)) < 0) {
        osal_debug_error_int("gpioSetPWMfrequency(x,v), x=", pin->addr);
    }
    else {
        pin_ll_set(pin, pin_get_prm(pin, PIN_INIT));
    }
}

