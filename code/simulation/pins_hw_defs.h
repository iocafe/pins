/**

  @file    simulation/pins_hw_defs.h
  @brief   Operating system specific defines for the pins library.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    8.1.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/


typedef void pin_interrupt_handler(void);

#ifdef PINS_OS_INT_HANDLER_HDRS
#define BEGIN_PIN_INTERRUPT_HANDLER(func_name) void func_name() {
#define END_PIN_INTERRUPT_HANDLER(func_name) }
#endif

#define PINS_SIMULATED_INTERRUPTS 1
