/**

  @file    arduino/pins_basics.c
  @brief   Pins library basic functionality.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    16.9.2019

  Copyright 2012 - 2019 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include <Arduino.h>
#include "pins.h"

#define LED_BUILTIN 2

/**
****************************************************************************************************

  @brief Initialize Hardware IO block.
  @anchor pins_setup

  The pins_setup() function...
  @return  None.

****************************************************************************************************
*/
void pins_setup(
    const Pin *pin_list,
    os_int flags)
{
    const Pin *pin;

    for (pin = pin_list; pin; pin = pin->board_next)
    {
        switch (pin->type)
        {
            case PIN_INPUT:
                pinMode(pin->addr, INPUT);
                break;

            case PIN_OUTPUT:
                pinMode(pin->addr, OUTPUT);
                break;

            case PIN_ANALOG_INPUT:
            case PIN_ANALOG_OUTPUT:
            case PIN_PWM:
            case PIN_TIMER:
                break;
        }
    }
}


/**
****************************************************************************************************

  @brief Set IO pin state.
  @anchor pin_set

  The pin_set() function...
  @return  None.

****************************************************************************************************
*/
void pin_set(
    const Pin *pin,
    os_int state)
{
    digitalWrite(pin->addr, state);
}


/**
****************************************************************************************************

  @brief Get IO pin state.
  @anchor pin_get

  The pin_get() function...
  @return  None.

****************************************************************************************************
*/
os_int pin_bin_get(
    const Pin *pin)
{
    return 0;
}
