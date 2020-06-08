/**

  @file    duino/pins_duino_basics.c
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
// #include <Arduino.h>
#include "pins.h"
#include "code/duino/pins_duino_gpio.h"
#include "code/duino/pins_duino_pwm.h"


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

  @param   pin Pointer to pin structure to initialize.
  @param   flags Reserved for future, set 0 for now.
  @return  None.

****************************************************************************************************
*/
void pin_ll_setup(
    const Pin *pin,
    os_int flags)
{
    os_int
        is_touch_sensor;

    if (pin->addr >=  0) switch (pin->type)
    {
        case PIN_INPUT:
            is_touch_sensor = pin_get_prm(pin, PIN_TOUCH);
            if (!is_touch_sensor)
            {
                pin_gpio_setup_input(pin);
            }
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
void OS_ISR_FUNC_ATTR pin_ll_set(
    const Pin *pin,
    os_int x)
{
    if (pin->addr >= 0) switch (pin->type)
    {
        case PIN_OUTPUT:
            digitalWrite(pin->addr, x);
            break;

        case PIN_PWM:
            /* ledcWrite(pin->bank, x); */
            break;

        case PIN_ANALOG_OUTPUT:
            /* dacWrite(pin->bank, x); */
            break;

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
  @return  Pin state, for example 0 or 1 for digital input.

****************************************************************************************************
*/
os_int OS_ISR_FUNC_ATTR pin_ll_get(
    const Pin *pin)
{
    if (pin->addr >= 0) switch (pin->type)
    {
        case PIN_INPUT:
            /* Touch sensor
             */
            if (pin->prm) {
                if (pin_get_prm(pin, PIN_TOUCH)) {
                    /* return touchRead(pin->addr); */
                    return 0;
                }
            }

            /* Normal digital input
             */
            return digitalRead(pin->addr);

        case PIN_ANALOG_INPUT:
            return analogRead(pin->addr);

        case PIN_OUTPUT:
        case PIN_PWM:
        case PIN_ANALOG_OUTPUT:
        case PIN_TIMER:
            break;
    }

    return 0;
}



