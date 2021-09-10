/**

  @file    esp32/pins_esp32_set_get.c
  @brief   Generic functions to set and get IO pin on ESP32.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    26.4.2021

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#define PINS_OS_INT_HANDLER_HDRS 1
#include "pinsx.h"
#ifdef OSAL_ESP32

#include "code/esp32/pins_esp32_pwm.h"
#include "code/esp32/pins_esp32_analog.h"

// #include "driver/ledc.h"
#include "driver/periph_ctrl.h"
#include "driver/adc.h"
#include "driver/dac.h"


/**
****************************************************************************************************

  @brief Set HW pin state.
  @anchor pin_ll_set

  The pin_ll_set() function...

  @param   pin Pointer to pin structure.
  @param   x Value to set, for example 0 or 1 for digital output.

****************************************************************************************************
*/
void OS_ISR_FUNC_ATTR pin_ll_set(
    const Pin *pin,
    os_int x)
{
    if (pin->addr >= 0) switch (pin->type)
    {
        case PIN_OUTPUT:
            gpio_set_level(pin->addr, x);
            break;

        case PIN_PWM:
            pin_pwm_set(pin, x);
            break;

        case PIN_ANALOG_OUTPUT:
            pin_write_analog_output(pin, x);
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

  The pin_ll_get() function read IO input from hardware

  @param   pin Pointer to pin structure.
  @param   state_bits Pointer to byte where to store state bits like OSAL_STATE_CONNECTED,
           OSAL_STATE_ORANGE, OSAL_STATE_YELLOW... Value OSAL_STATE_UNCONNECTED indicates not
           connected (= unknown value).
  @return  Pin value, for example 0 or 1 for digital input.

****************************************************************************************************
*/
os_int OS_ISR_FUNC_ATTR pin_ll_get(
    const Pin *pin,
    os_char *state_bits)
{
    if (pin->addr >= 0) switch (pin->type)
    {
        case PIN_INPUT:
            /* Touch sensor
             */
            if (pin->prm) {
                if (pin_get_prm(pin, PIN_TOUCH)) {
                    *state_bits = OSAL_STATE_CONNECTED;
#ifndef OSAL_ESPIDF_FRAMEWORK
                    return touchRead(pin->addr);
#else
                    return 0;
#endif                            
                }
            }

            /* Normal digital input
             */
            *state_bits = OSAL_STATE_CONNECTED;
#ifndef OSAL_ESPIDF_FRAMEWORK
            return digitalRead(pin->addr);
#else
            return 0;
#endif                            

        case PIN_ANALOG_INPUT:
            return pin_read_analog_input(pin, state_bits);

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
