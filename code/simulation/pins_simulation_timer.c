/**

  @file    simulation/pins_simulation_timer.c
  @brief   Hardware timer interrupts.
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
#ifdef PINS_SIMULATE_HW

void pin_timer_attach_interrupt(
    const struct Pin *pin,
    pinTimerParams *prm)
{
#if PINS_SIMULATED_INTERRUPTS

    if (pin->int_conf == OS_NULL)
    {
        osal_debug_error("pin_timer_attach_interrupt: pin->int_conf is NULL");
        return;
    }

    /** Store the interrupt handler function pointer and flags (when to trigger interrupts)
        for simulation.
     */
    pin->int_conf->int_handler_func = prm->int_handler_func;
    // pin->int_conf->flags = prm->flags;
    os_get_timer(&pin->int_conf->hit_timer);
#endif
}


#if PINS_SIMULATED_INTERRUPTS
/**
****************************************************************************************************

  @brief Trigger a simulalated timer interrupt periodically.
  @anchor pin_timer_simulate_interrupt

  For PC simulation only.

  @param   pin The timer pin structure.
  @return  None.

****************************************************************************************************
*/
void pin_timer_simulate_interrupt(
    const struct Pin *pin)
{
    os_timer ti;
    os_int x, period_ms;

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

    os_get_timer(&ti);
    x = pin_get_frequency(pin, 50);
    period_ms = 1;
    if (x > 0) period_ms = (os_int)(1000.0 / x + 0.5);
    if (period_ms < 1) period_ms = 1;

    if (os_has_elapsed_since(&pin->int_conf->hit_timer, &ti, period_ms))
    {
        pin->int_conf->int_handler_func();
        pin->int_conf->hit_timer = ti;
    }
}
#endif
#endif
