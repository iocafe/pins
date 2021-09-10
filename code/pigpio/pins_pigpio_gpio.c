/**

  @file    pigpio/pins_pigpio_gpio.c
  @brief   GPIO pins.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    26.4.2021

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "pins.h"
#ifdef PINS_PIGPIO
#include <pigpio.h>


/**
****************************************************************************************************

  @brief Configure a pin as digital input.
  @anchor pin_gpio_setup_input

  The pin_gpio_setup_input() function...

  @param   pin Pointer to pin structure.
  @return  None.

****************************************************************************************************
*/
void pin_gpio_setup_input(
    const Pin *pin)
{
    unsigned pud;

    if (gpioSetMode((unsigned)pin->addr, PI_INPUT)) {
        osal_debug_error_int("gpioSetMode(x,PI_INPUT) failed, x=", pin->addr);
    }
    else
    {
        pud = PI_PUD_OFF;
        if (pin_get_prm(pin, PIN_PULL_DOWN)) {
            pud = PI_PUD_DOWN;
        }
        else if (pin_get_prm(pin, PIN_PULL_UP)) {
            pud = PI_PUD_UP;
        }
        if (gpioSetPullUpDown((unsigned)pin->addr, pud)) {
            osal_debug_error_int("gpioSetPullUpDown(x,pud) failed, x=", pin->addr);
        }
    }
}


/**
****************************************************************************************************

  @brief Configure a pin as digital output.
  @anchor pin_gpio_setup_output

  The pin_gpio_setup_output() function...

  @param   pin Pointer to pin structure.
  @return  None.

****************************************************************************************************
*/
void pin_gpio_setup_output(
    const Pin *pin)
{
    if (gpioSetMode((unsigned)pin->addr, PI_OUTPUT)) {
        osal_debug_error_int("gpioSetMode(x, PI_OUTPUT) failed, x=", pin->addr);
    }
}


/**
****************************************************************************************************

  @brief Attach an interrupt to a GPIO pin.
  @anchor pin_gpio_attach_interrupt

  The pin_gpio_attach_interrupt() function to set an interrupt handler function on a pin by pin basis.

  There may be HW specific parameters and limitations for reserving interrupt channels, etc.
  The HW specific parameters are in JSON file for the hardware.

  @param   pin The GPIO pin structure.
  @param   prm Parameter structure, contains pointer to interrupt handler function that
           will be called every time the interrupt is triggered.
  @param   flags Flags to specify when interrupt is triggered.
  @return  None.

****************************************************************************************************
*/
void pin_gpio_attach_interrupt(
    const struct Pin *pin,
    pinInterruptParams *prm)
{
    // IN PI WE USE ALERT FUNCTIONS, NOT INTERRUPTS
    // int gpioSetAlertFunc(unsigned user_gpio, gpioAlertFunc_t f)
    // Registers a function to be called (a callback) when the specified GPIO changes state.
    // OR gpioSetAlertFuncEx(unsigned user_gpio, gpioAlertFuncEx_t f, void *userdata)
    // OR nt gpioSetISRFuncEx(unsigned gpio, unsigned edge, int timeout, gpioISRFuncEx_t f, void *userdata)

}


/**
****************************************************************************************************

  @brief Detach interrupt from GPIO pin.
  @anchor pin_gpio_detach_interrupt

  You can call pin_gpio_detach_interrupt() function when you no longer want to receive interrupts
  related to the pin.

  @param   pin Pointer to the pin structure.
  @return  None.

****************************************************************************************************
*/
void pin_gpio_detach_interrupt(
    const struct Pin *pin)
{
}

#endif
