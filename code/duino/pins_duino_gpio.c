/**

  @file    duino/pins_duino_gpio.c
  @brief   GPIO pins.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    21.4.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "pins.h"
#ifdef OSAL_ARDUINO


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
    int mode = INPUT;
    if (pin_get_prm(pin, PIN_PULL_UP)) mode = INPUT_PULLUP;
    /* if (pin_get_prm(pin, PIN_PULL_DOWN)) mode = INPUT_PULLDOWN; */
    pinMode(pin->addr, mode);
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
    pinMode(pin->addr, OUTPUT);
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
    int mode;

    switch (prm->flags & PINS_INT_CHANGE)
    {
        case PINS_INT_FALLING: mode = FALLING; break;
        case PINS_INT_RISING:  mode = RISING; break;
        default:
        case PINS_INT_CHANGE: mode = CHANGE; break;
    }

    attachInterrupt(pin->addr, prm->int_handler_func, mode);
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
    detachInterrupt(pin->addr);
}

#endif
