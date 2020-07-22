/**

  @file    pigpio/pins_hw_defs.h
  @brief   Raspberry PI PIGPIO specific defines for the pins library.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    21.7.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/

/* Interrupt handler function type.
 */
// typedef void pin_interrupt_handler(void *arg);

/* Simulated interrupts on PIGPIO.
 */
#define PINS_SIMULATION 0
#define PINS_SIMULATED_INTERRUPTS 1
