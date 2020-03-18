/**

  @file    arduino/pins_hw_defs.h
  @brief   Operating system specific defines for the pins library.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    17.3.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/

/* Operating system/platform specific defines for interrupt handlers
 */
#ifdef ESP_PLATFORM
  #ifdef PINS_OS_INT_HANDLER_HDRS
  #include "esp_attr.h"
  #endif
  // typedef void IRAM_ATTR pin_interrupt_handler(void);
  typedef void pin_interrupt_handler(void);
  #define BEGIN_PIN_INTERRUPT_HANDLER(name) void IRAM_ATTR name() {
  #define END_PIN_INTERRUPT_HANDLER }
  #define PINS_SIMULATED_INTERRUPTS 0
#endif
