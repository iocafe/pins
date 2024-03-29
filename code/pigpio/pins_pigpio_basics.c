/**

  @file    pigpio/pins_pigpio_basics.c
  @brief   Pins library basic functionality.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    21.7.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#define PINS_OS_INT_HANDLER_HDRS 1
#include "pinsx.h"
#ifdef PINS_PIGPIO
#include "code/pigpio/pins_pigpio_gpio.h"
#include "code/pigpio/pins_pigpio_pwm.h"
#include <pigpio.h>

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
    int s, cfg;

    /* Do not use PIGPIO signal handle, eosal app has it's own.
     */
    cfg = gpioCfgGetInternals();
    cfg |= PI_CFG_NOSIGHANDLER;  // (1<<10)
    gpioCfgSetInternals(cfg);

    s = gpioInitialise();
    if (s < 0) {
        osal_debug_error("gpioInitialise() failed. PIGPIO may need root priviliges.");
        return OSAL_STATUS_FAILED;
    }
    else {
        osal_trace_int("pigpio version ", s);
        return OSAL_SUCCESS;
    }
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
    gpioTerminate();
}
#endif


/**
****************************************************************************************************

  @brief Initialize hardware IO pin.
  @anchor pin_ll_setup

  The pin_ll_setup() function...

  @param   pin Pointer to pin structure to initialize.
  @param   flags Reserved for future, set 0 for now.
  @return  None.

****************************************************************************************************
*/
void pin_ll_setup(
    const Pin *pin,
    os_int flags)
{
    if (pin->addr >= 0) switch (pin->type)
    {
        case PIN_INPUT:
            pin_gpio_setup_input(pin);
            break;

        case PIN_OUTPUT:
            pin_gpio_setup_output(pin);
            break;

        case PIN_PWM:
            pin_pwm_setup(pin);
            break;

        case PIN_ANALOG_INPUT:
        case PIN_ANALOG_OUTPUT:
        case PIN_TIMER:
            break;
    }
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

  @brief Set HW pin state.
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
    if (pin->addr >= 0) switch (pin->type)
    {
        case PIN_OUTPUT:
            if (gpioWrite(pin->addr, x ? 1 : 0)) {
                osal_debug_error_int("gpioWrite(x,v) failed, x=", pin->addr);
            }
            break;

        case PIN_PWM:
            if (gpioPWM(pin->addr, (unsigned)x)) { // x is dutycycle, defaults to max 255?
                osal_debug_error_int("gpioPWM(x,v) failed, x=", pin->addr);
            }
            break;

        case PIN_ANALOG_OUTPUT:
        case PIN_INPUT:
        case PIN_ANALOG_INPUT:
        case PIN_TIMER:
            break;
    }
}


/**
****************************************************************************************************

  @brief Get HW pin state.
  @anchor pin_ll_get

  The pin_ll_get() function gets IO pin state from hardware

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
    os_int r;
    if (pin->addr >= 0) switch (pin->type)
    {
        case PIN_INPUT:
            r = gpioRead(pin->addr);
            if (r < 0) {
                osal_debug_error_int("gpioRead(x) failed, x=", pin->addr);
                *state_bits = OSAL_STATE_RED;
                return 0;
            }
            *state_bits = OSAL_STATE_CONNECTED;
            return r;

        case PIN_ANALOG_INPUT:
        case PIN_OUTPUT:
        case PIN_PWM:
        case PIN_ANALOG_OUTPUT:
        case PIN_TIMER:
            break;
    }

    *state_bits = OSAL_STATE_NO_READ_SUPPORT;
    return 0;
}

#endif

