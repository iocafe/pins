/**

  @file    arduino/pins_esp32_timer.c
  @brief   Interrups and handlers.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    1.4.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "pins.h"

void pin_setup_timer(
    const struct Pin *pin,
    pinTimerParams *prm)
{
#if PINS_SIMULATED_INTERRUPTS

    if (pin->int_conf == OS_NULL)
    {
        osal_debug_error("pin_setup_timer: pin->int_conf is NULL");
        return;
    }

    /** Store the interrupt handler function pointer and flags (when to trigger interrupts)
        for simulation.
     */
    pin->int_conf->int_handler_func = prm->int_handler_func;
    pin->int_conf->flags = prm->flags;
    os_get_timer(&pin->int_conf->hit_timer);
#endif
}


#if PINS_SIMULATED_INTERRUPTS
/**
****************************************************************************************************

  @brief Trigger a simulalated timer interrupt periodically.
  @anchor pin_simulate_timer

  For PC simulation only.

  @param   pin The timer pin structure.
  @return  None.

****************************************************************************************************
*/
void pin_simulate_timer(
    const struct Pin *pin)
{
    os_timer ti;
    os_int x, period_ms;

    /* If pin is not configured for interrupts.
     */
    if (pin->int_conf == OS_NULL)
    {
        osal_debug_error("pin_simulate_interrupt: NULL int_conf pointer");
        return;
    }

    /* If interrupt handler not set, just return.
     */
    if (pin->int_conf->int_handler_func == OS_NULL) return;

    os_get_timer(&ti);
    x = pin_get_prm(pin, PIN_FREQENCY);
    if (x <= 0) x = 1000 * pin_get_prm(pin, PIN_FREQENCY_KHZ);
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
