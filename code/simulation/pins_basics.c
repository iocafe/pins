/**

  @file    simulation/pins_basics.c
  @brief   Pins library basic functionality.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    21.4.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "pins.h"
#include <stdlib.h> /* for rand() */


/**
****************************************************************************************************

  @brief Initialize hardware IO library.
  @anchor pins_ll_initialize

  @return  None.

****************************************************************************************************
*/
void pins_ll_initialize(
    void)
{
}


/**
****************************************************************************************************

  @brief Initialize hardware IO pin.
  @anchor pin_ll_setup

  The pin_ll_setup() function...
  @param   pin Pin to initialize.
  @param   flags Reserved for future, set 0 for now.
  @return  None.

****************************************************************************************************
*/
void pin_ll_setup(
    const Pin *pin,
    os_int flags)
{
}


/**
****************************************************************************************************

  @brief Set IO pin state.
  @anchor pin_ll_set

  The pin_ll_set() function...
  @return  None.

****************************************************************************************************
*/
void pin_ll_set(
    const Pin *pin,
    os_int x)
{
    osal_trace_int("~Setting pin addr ", pin->addr);
    osal_trace_int(" to value ", x);
}


/**
****************************************************************************************************

  @brief Get IO pin state.
  @anchor pin_ll_get

  The pin_ll_get() function...
  @return  None.

****************************************************************************************************
*/
os_int pin_ll_get(
    const Pin *pin)
{
    switch (pin->type)
    {
        case PIN_INPUT:
            return (os_int)osal_rand(0, 1);

        default:
            return (os_int)osal_rand(0, 65535);
    }
}
