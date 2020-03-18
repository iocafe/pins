/**

  @file    arduino/pins_interrupt.c
  @brief   Interrups and handlers.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    16.3.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "pins.h"


/**
****************************************************************************************************

  @brief Attach and interrupt to a GPIO pin.
  @anchor pin_attach_interrupt

  The pin_attach_interrupt() function to set an interrupt handler function on a pin by pin basis.

  There may be HW specific parameters and limitations for reserving interrupt channels, etc.
  The HW specific parameters are in JSON file for the hardware.

  @param   pin The GPIO pin structure.
  @param   prm Parameter structure, contains pointer to interrupt handler functio that
           will be called every time the interrupt is triggered.
  @param   flags Flags to specify when interrupt is triggered.
  @return  None.

****************************************************************************************************
*/
void pin_attach_interrupt(
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
  @anchor pin_detach_interrupt

  You can call pin_detach_interrupt() function when you no longer want to receive interrupts
  related to the pin.

  @return  None.

****************************************************************************************************
*/
void pin_detach_interrupt(
    const struct Pin *pin)
{
    detachInterrupt(pin->addr);
}
