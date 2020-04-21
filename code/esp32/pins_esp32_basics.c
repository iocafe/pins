/**

  @file    esp32/pins_esp32_basics.c
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
#include "code/esp32/pins_esp32_gpio.h"
#include "code/esp32/pins_esp32_pwm.h"

#ifdef ESP_PLATFORM
#include "driver/ledc.h"
#include "driver/periph_ctrl.h"
#endif


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
    periph_module_enable(PERIPH_LEDC_MODULE);
    gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
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


/**
****************************************************************************************************

  @brief Set IO pin state.
  @anchor pin_ll_set

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
#ifdef ESP_PLATFORM
            gpio_set_level(pin->addr, x);
#else
            digitalWrite(pin->addr, x);
#endif
            break;

        case PIN_PWM:
#ifdef ESP_PLATFORM
            /* Not thread safe: PWM channel duty must be modified only from one thread at a time.
             */
            ledc_set_duty(LEDC_HIGH_SPEED_MODE, pin->bank, (uint32_t)x);
            ledc_update_duty(LEDC_HIGH_SPEED_MODE, pin->bank);
#else
            ledcWrite(pin->bank, x);
#endif
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



