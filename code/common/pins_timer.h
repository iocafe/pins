/**

  @file    common/pins_timer.h
  @brief   Timer interrupts.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    2.4.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/

struct Pin;

/* Timer flags
 */
#define PIN_TIMER_START 1


/* Parameter structure for pin_attach_interrupt() function.
 */
typedef struct pinTimerParams
{
    /** Pointer to interrupt handler function.
     */
    pin_interrupt_handler *int_handler_func;

    /* Flags for the timer.
     */
    os_int flags;
}
pinTimerParams;


void pin_setup_timer(
    const struct Pin *pin,
    pinTimerParams *prm);


#if PINS_SIMULATED_INTERRUPTS
/* Trigger a simulalated timer interrupt periodically.
 */
void pin_simulate_timer(
    const struct Pin *pin);
#endif
