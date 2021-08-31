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
#include "pinsx.h"
#ifdef PINS_SIMULATE_HW

#include <stdlib.h> /* for rand() */

/**
****************************************************************************************************

  @brief Initialize hardware IO library.
  @anchor pins_ll_initialize_lib

  @return  If IO library is successfully initialized, the function returns OSAL_SUCCESS.
           Other return values indicate an error.

****************************************************************************************************
*/
osalStatus pins_ll_initialize_lib(
    void)
{
    return OSAL_SUCCESS;
}


#if OSAL_PROCESS_CLEANUP_SUPPORT
/**
****************************************************************************************************

  @brief Clean up resources allocated by IO hardware library.
  @anchor pins_ll_shutdown_lib

  @return  None.

****************************************************************************************************
*/
void pins_ll_shutdown_lib(
    void)
{
}
#endif


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


#if OSAL_PROCESS_CLEANUP_SUPPORT
/**
****************************************************************************************************

  @brief Release any resources allocated for IO hardware "pin".
  @anchor pin_ll_shutdown

  @param   pin Pin to initialize.
  @return  None.

****************************************************************************************************
*/
void pin_ll_shutdown(
    const Pin *pin)
{
}
#endif


/**
****************************************************************************************************

  @brief Set IO pin state.
  @anchor pin_ll_set

  The pin_ll_set() function...
  @param   pin Pointer to pin structure.
  @param   x Value to set, for example 0 or 1 for digital output.
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

  @param   pin Pointer to pin structure.
  @param   state_bits Pointer to byte where to store state bits like OSAL_STATE_CONNECTED,
           OSAL_STATE_ORANGE, OSAL_STATE_YELLOW... Value OSAL_STATE_UNCONNECTED indicates not
           connected (= unknown value).
  @return  Pin value, for example 0 or 1 for digital input.

****************************************************************************************************
*/
os_int pin_ll_get(
    const Pin *pin,
    os_char *state_bits)
{
    switch (pin->type)
    {
        case PIN_INPUT:
            *state_bits = OSAL_STATE_CONNECTED;
            return (os_int)osal_rand(0, 1);

        default:
            *state_bits = OSAL_STATE_CONNECTED;
            return (os_int)osal_rand(0, 65535);
    }
}

#endif
