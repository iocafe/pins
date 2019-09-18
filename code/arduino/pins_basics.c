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
    os_int
        is_touch_sensor,
        frequency_hz,
        resolution_bits,
        initial_state;

    for (pin = pin_list; pin; pin = pin->board_next)
    {
        if (pin->addr < 0) continue;
        switch (pin->type)
        {
            case PIN_INPUT:
                is_touch_sensor = pin_get_prm(pin, PIN_TOUCH);

                if (!is_touch_sensor)
                {
                    pinMode(pin->addr, pin_get_prm(pin, PIN_PULL_UP) ? INPUT_PULLUP : INPUT);
                }
                break;

            case PIN_OUTPUT:
                pinMode(pin->addr, OUTPUT);
                break;

            case PIN_PWM:
                frequency_hz = pin_get_prm(pin, PIN_FREQENCY);
                if (!frequency_hz) frequency_hz = 50; /* Default servo frequency */
                resolution_bits = pin_get_prm(pin, PIN_RESOLUTION);
                if (!resolution_bits) resolution_bits = 12;
                initial_state  = pin_get_prm(pin, PIN_INIT);
                ledcSetup(pin->bank, frequency_hz, resolution_bits);
                ledcAttachPin(pin->addr, pin->bank);
                ledcWrite(pin->bank, initial_state);
                break;

            case PIN_ANALOG_INPUT:
            case PIN_ANALOG_OUTPUT:
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
    if (pin->addr < 0) return;
    switch (pin->type)
    {
        case PIN_OUTPUT:
            digitalWrite(pin->addr, state);
            break;

        case PIN_PWM:
            ledcWrite(pin->bank, state);
            break;

    case PIN_ANALOG_OUTPUT:
#ifdef ESP_PLATFORM
            dacWrite(pin->bank, state);
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

  The pin_get() function...
  @return  None.

****************************************************************************************************
*/
os_int pin_get(
    const Pin *pin)
{
    if (pin->addr < 0) return 0;
    switch (pin->type)
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
}
