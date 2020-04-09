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
  @anchor pin_gpio_attach_interrupt

  The pin_gpio_attach_interrupt() function to set an interrupt handler function on a pin by pin basis.

  There may be HW specific parameters and limitations for reserving interrupt channels, etc.
  The HW specific parameters are in JSON file for the hardware.

  @param   pin The GPIO pin structure.
  @param   prm Parameter structure, contains pointer to interrupt handler functio that
           will be called every time the interrupt is triggered.
  @param   flags Flags to specify when interrupt is triggered.
  @return  None.

****************************************************************************************************
*/
void pin_gpio_attach_interrupt(
    const struct Pin *pin,
    pinInterruptParams *prm)
{
#if PINS_SIMULATED_INTERRUPTS

    if (pin->int_conf == OS_NULL)
    {
        osal_debug_error("pin_gpio_attach_interrupt: No \'interrupt\' attribute in JSON, etc");
        return;
    }

    /** Store the interrupt handler function pointer and flags (when to trigger interrupts)
        for simulation.
     */
    pin->int_conf->int_handler_func = prm->int_handler_func;
    pin->int_conf->flags = prm->flags;
#endif
}


/**
****************************************************************************************************

  @brief Detach interrupt from GPIO pin.
  @anchor pin_gpio_detach_interrupt

  You can call pin_gpio_detach_interrupt() function when you no longer want to receive interrupts
  related to the pin.

  @return  None.

****************************************************************************************************
*/
void pin_gpio_detach_interrupt(
    const struct Pin *pin)
{
    if (pin->int_conf == OS_NULL)
    {
        osal_debug_error("pin_gpio_detach_interrupt: No \'interrupt\' attribute in JSON, etc");
        return;
    }

    if (pin->int_conf->int_handler_func == OS_NULL)
    {
        osal_debug_error("pin_gpio_detach_interrupt: Interrupt was not attached to pin?");
        return;
    }

    pin->int_conf->int_handler_func = OS_NULL;
}


#if PINS_SIMULATED_INTERRUPTS
/**
****************************************************************************************************

  @brief Trigger a simulalated interrupt if flags match to x change.
  @anchor pin_gpio_simulate_interrupt

  The pin_gpio_simulate_interrupt function triggers a simulated interrupt if:
  - x is zero and PINS_INT_FALLING flag is set (included in PINS_INT_CHANGE).
  - x is nonzero and PINS_INT_RISING flag is set (included in PINS_INT_CHANGE).

  For PC simulation only.

  @param   pin The GPIO pin structure.
  @param   x New pin state.
  @return  None.

****************************************************************************************************
*/
void pin_gpio_simulate_interrupt(
    const struct Pin *pin,
    os_int x)
{
    os_short flags;

    /* If pin is not configured for interrupts.
     */
    if (pin->int_conf == OS_NULL)
    {
        osal_debug_error("pin_gpio_simulate_interrupt: NULL int_conf pointer");
        return;
    }

    /* If interrupt handler not set, just return.
     */
    if (pin->int_conf->int_handler_func == OS_NULL) return;

    /* If new signal value matches rising/falling edge flag, call interrupt handler.
     */
    flags = pin->int_conf->flags;
    if (((flags & PINS_INT_FALLING) && x == 0) ||
        ((flags & PINS_INT_RISING) && x != 0))
    {
        pin->int_conf->int_handler_func();
    }
}
#endif
