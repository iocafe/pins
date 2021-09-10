/**

  @file    common/pins_timer.h
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
#pragma once
#ifndef PINS_TIMER_H_
#define PINS_TIMER_H_
#include "pins.h"

struct Pin;

/* Timer flags
 */
// #define PIN_TIMER_START 1


/* Parameter structure for pin_gpio_attach_interrupt() function.
 */
typedef struct pinTimerParams
{
    /** Pointer to interrupt handler function.
     */
    pin_interrupt_handler *int_handler_func;

    /* Flags for the timer.
     */
    // os_int flags;
}
pinTimerParams;

/* Start timer and attach interrupt.
 */
void pin_timer_attach_interrupt(
    const struct Pin *pin,
    pinTimerParams *prm);

/* Detach interrupt from timer (may stop timer).
 */
void pin_timer_detach_interrupt(
    const struct Pin *pin);

#if PINS_SIMULATED_INTERRUPTS
/* Trigger a simulalated timer interrupt periodically.
 */
void pin_timer_simulate_interrupt(
    const struct Pin *pin);
#endif

#endif
