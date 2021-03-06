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
#define PINS_OS_INT_HANDLER_HDRS 1
#include "pinsx.h"
#include "code/esp32/pins_esp32_gpio.h"
#include "code/esp32/pins_esp32_pwm.h"

#include "Arduino.h"
#include "driver/ledc.h"
#include "driver/periph_ctrl.h"

/**
****************************************************************************************************

  @brief Initialize hardware IO library.
  @anchor pins_ll_initialize_lib

  @return  None.

****************************************************************************************************
*/
void pins_ll_initialize_lib(
    void)
{
    periph_module_enable(PERIPH_LEDC_MODULE);

    /* pekka 9.8.2020: This seems to be right one. We get warning when camera is started
       that grpio_isr_service is already installed. But ignoring that seems to be ok.
       This call is necessary, strange behaviour without it.
     */
    gpio_install_isr_service(0 /* 0=default */);

    /* WAS (this code is rejected now)
    #if PINS_IS_ESP32_CAMERA==0
        gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
    #else */
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

    if (pin->addr >= 0) switch (pin->type)
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
            gpio_set_level(pin->addr, x);
            break;

        case PIN_PWM:
            /* Not thread safe: PWM channel duty must be modified only from one thread at a time.
             */
            ledc_set_duty(LEDC_HIGH_SPEED_MODE, pin->bank, (uint32_t)x);
            ledc_update_duty(LEDC_HIGH_SPEED_MODE, pin->bank);
            break;

    case PIN_ANALOG_OUTPUT:
            dacWrite(pin->bank, x);
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
                    return touchRead(pin->addr);
                }
            }

            /* Normal digital input
             */
            *state_bits = OSAL_STATE_CONNECTED;
            return digitalRead(pin->addr);

        case PIN_ANALOG_INPUT:
            *state_bits = OSAL_STATE_CONNECTED;
            return analogRead(pin->addr);

        case PIN_OUTPUT:
        case PIN_PWM:
        case PIN_ANALOG_OUTPUT:
        case PIN_TIMER:
            break;
    }

    *state_bits = OSAL_STATE_UNCONNECTED;
    return 0;
}



