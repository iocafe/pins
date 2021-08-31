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
#pragma once
#ifndef PINS_HW_DEFS_H_
#define PINS_HW_DEFS_H_
#include "pins.h"

/* Use PIGPIO code, this can be checked by #ifdef PINS_PIGPIO
 */
#define PINS_PIGPIO 1

/* Interrupt handler (actually callback for raspberry) function type.
 */
typedef void pin_interrupt_handler(void *arg);

#ifdef PINS_OS_INT_HANDLER_HDRS
#define PIN_INTERRUPT_HANDLER_PROTO(name) void name(void)
#define BEGIN_PIN_INTERRUPT_HANDLER(func_name) void func_name() {
#define END_PIN_INTERRUPT_HANDLER(func_name) }
#define TIMER_INTERRUPT_HANDLER_PROTO(name) void name(void)
#define BEGIN_TIMER_INTERRUPT_HANDLER(func_name) void func_name() {
#define END_TIMER_INTERRUPT_HANDLER(func_name) }
#endif

/* Simulated interrupts on PIGPIO.
 */
#define PINS_SIMULATION 0
#define PINS_SIMULATED_INTERRUPTS 0

#endif
