/**

  @file    esp32/pins_esp32_init.c
  @brief   IO initialization and generic pin configuration for ESP32.
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
#ifdef OSAL_ESP32

#include "code/esp32/pins_esp32_gpio.h"
#include "code/esp32/pins_esp32_pwm.h"
#include "code/esp32/pins_esp32_analog.h"

#include "driver/ledc.h"
#include "driver/periph_ctrl.h"
// #include "driver/adc.h"
// #include "driver/dac.h"

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
    periph_module_enable(PERIPH_LEDC_MODULE);

    /* pekka 9.8.2020: This seems to be right one. We may get a warning when camera is started
       that grpio_isr_service is already installed. But ignoring that seems to be ok.
       This call is necessary, strange behaviour without it.
     */
    gpio_install_isr_service(0 /* 0=default */);

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
            if (!is_touch_sensor) {
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
            pin_setup_analog_input(pin);
            break;

        case PIN_ANALOG_OUTPUT:
            pin_setup_analog_output(pin);
            break;
            
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

#endif