/**

  @file    simulation/pins_interrupt.c
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

  @param  pin The GPIO pin, structure.
  @param  prm Parameter structure, contains pointer to interrupt handler functio that
          will be called every time the interrupt is triggered.
  @param  flags Flags to specify how interrupt is triggered.

  There may be HW specific parameters and limitations for reserving interrupt channels, etc.
  The HW specific parameters are in JSON file for the hardware.

  @return  None.

****************************************************************************************************
*/
void pin_attach_interrupt(
    const struct Pin *pin,
    pinInterruptParams *prm)
{
    // osal_trace_int("~Setting pin addr ", pin->addr);

#if PINS_SIMULATED_INTERRUPTS
    /** Store the interrupt handler function pointer and flags (when to trigger interrupts)
        for simulation.
     */
    pin->int_handler_func = prm->int_handler_func;
    pin->int_flags = prm->flags;
#endif

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

}

