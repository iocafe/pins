/**

  @file    esp32/pins_hw_defs.h
  @brief   Operating system specific defines for the pins library.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    21.4.2020

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
    #include "driver/periph_ctrl.h"
    #include "driver/timer.h"
  #endif
  typedef void pin_interrupt_handler(void *arg);

  #ifdef PINS_OS_INT_HANDLER_HDRS
    #define PINS_LOCK_NAME(name) pins_lock_##name

    #define BEGIN_PIN_INTERRUPT_HANDLER(name) \
    static portMUX_TYPE DRAM_ATTR PINS_LOCK_NAME(name) = portMUX_INITIALIZER_UNLOCKED; \
    void IRAM_ATTR name(void *arg) { \
      portENTER_CRITICAL_ISR(&PINS_LOCK_NAME(name));

    #define END_PIN_INTERRUPT_HANDLER(name) \
      portEXIT_CRITICAL_ISR(&PINS_LOCK_NAME(name)); }

  #if 0
  // FOR EXP-IDF 4 USE SOMETHING LIKE THIS
    #define BEGIN_TIMER_INTERRUPT_HANDLER(name) \
      void IRAM_ATTR name(void *arg) { \
      int group_nr = 0xF & (int)arg; \
      int timer_nr = ((int)arg) >> 4; \
      timer_spinlock_take(group_nr); \
      uint32_t timer_intr = timer_group_get_intr_status_in_isr(group_nr); \
      uint64_t timer_counter_value = timer_group_get_counter_value_in_isr(group_nr, timer_nr); \
      timer_group_clr_intr_status_in_isr(group_nr, timer_nr); \
      timer_group_enable_alarm_in_isr(group_nr, timer_nr);

    #define END_TIMER_INTERRUPT_HANDLER(name) \
      timer_spinlock_give(group_nr); }
  #endif

  #if 1
  // FOR EXP-IDF 3 USE SOMETHING LIKE THIS This only works for timer group TIMERG0, should work for both TIMER_0 and Timer1
    #define BEGIN_TIMER_INTERRUPT_HANDLER(name) \
    void IRAM_ATTR name(void *arg) { \
      int timer_nr = ((int)arg) >> 4; \
      if (timer_nr == 0) TIMERG0.int_clr_timers.t0 = 1; \
      else TIMERG0.int_clr_timers.t1 = 1; \
      TIMERG0.hw_timer[timer_nr].config.alarm_en = 1;

    #define END_TIMER_INTERRUPT_HANDLER(name) }
  #endif

#endif
  #define PINS_SIMULATION 0
  #define PINS_SIMULATED_INTERRUPTS 0
#else
   typedef void pin_interrupt_handler(void);
#endif
