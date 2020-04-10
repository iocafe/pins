/**

  @file    esp32/pins_hw_defs.h
  @brief   Operating system specific defines for the pins library.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    7.4.2020

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
    #include "driver/timer.h"
  #endif
  typedef void pin_interrupt_handler(void *ulle);

  #ifdef PINS_OS_INT_HANDLER_HDRS
    #define PINS_LOCK_NAME(name) pins_lock_##name

    #define BEGIN_PIN_INTERRUPT_HANDLER(name) \
    static portMUX_TYPE DRAM_ATTR PINS_LOCK_NAME(name) = portMUX_INITIALIZER_UNLOCKED; \
    \
    void IRAM_ATTR name(void *ulle) { \
        portENTER_CRITICAL_ISR(&PINS_LOCK_NAME(name));

    #define END_PIN_INTERRUPT_HANDLER(name) \
      portEXIT_CRITICAL_ISR(&PINS_LOCK_NAME(name)); }

    #define BEGIN_TIMER_INTERRUPT_HANDLER(name) \
    void IRAM_ATTR name(void *ulle) {

    #define END_TIMER_INTERRUPT_HANDLER(name) \
      TIMERG0.int_clr_timers.t0 = 1; \
      TIMERG0.hw_timer[0].config.alarm_en = 1; }
  #endif

  #define PINS_SIMULATED_INTERRUPTS 0
#else
   typedef void pin_interrupt_handler(void);
#endif
