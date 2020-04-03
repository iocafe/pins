/**

  @file    arduino/pins_setget.c
  @brief   Pins library basic functionality.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    8.1.2020

  THESE CAN BE CALLED FROM INTERRUPT HANDLER.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include <Arduino.h>
#include "pins.h"

/**
****************************************************************************************************

  @brief Set IO pin state.
  @anchor pin_ll_set

  CAN BE CALLED FROM INTERRUPT HANDLER.

  The pin_ll_set() function...
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
            ledcWrite(pin->bank, x);
            break;

    case PIN_ANALOG_OUTPUT:
#ifdef ESP_PLATFORM
            dacWrite(pin->bank, x);
            break;
#endif

        case PIN_INPUT:
        case PIN_ANALOG_INPUT:
        case PIN_TIMER:
            break;
    }
}


/**
****************************************************************************************************

  @brief Get IO pin state.
  @anchor pin_get

  CAN BE CALLED FROM INTERRUPT HANDLER.

  The pin_ll_get() function...
  @return  None.

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
                    return touchRead(pin->addr);
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
