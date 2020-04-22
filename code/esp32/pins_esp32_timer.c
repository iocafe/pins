/**

  @file    esp32/pins_esp32_timer.c
  @brief   Timer interrups.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    21.4.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "pins.h"
#include "driver/timer.h"

/* Forward referred static functions.
 */
static void pin_timer_global_interrupt_control(
    os_boolean enable,
    void *context);

static void pin_timer_set_interrupt_enable_flag(
   const struct Pin *pin,
   os_boolean enable,
   os_int flag);

static void pin_timer_control_interrupt(
    const struct Pin *pin);


/**
****************************************************************************************************

  @brief Attach an interrupt to a timer.
  @anchor pin_timer_attach_interrupt

  The pin_timer_attach_interrupt() function sets an interrupt handler function for
  timer interrupts. The function also configures the timer.

  There may be HW specific parameters and limitations for reserving timers, channels, etc.
  The HW specific parameters are in JSON file for the hardware.

  @param   pin Pointer to the pin structure for the timer.
  @param   prm Parameter structure, contains pointer to interrupt handler function that
           will be called at timer hit.
  @return  None.

****************************************************************************************************
*/
void pin_timer_attach_interrupt(
    const struct Pin *pin,
    pinTimerParams *prm)
{
#ifdef ESP_PLATFORM
    timer_config_t config;
    os_int timer_nr, timer_group, frequency_hz, divisor, target_count, count;
    const os_int hw_clock_frequency = 80000000; // 80 MHz
    os_boolean enable;

    intr_handle_t s_timer_handle;

    timer_nr = pin_get_prm(pin, PIN_TIMER_SELECT);
    timer_group = pin_get_prm(pin, PIN_TIMER_GROUP_SELECT);
    frequency_hz = pin_get_frequency(pin, 50);
    target_count = 10000;

    divisor = 1;
    if (frequency_hz >= 1)
    {
        divisor = hw_clock_frequency / (frequency_hz * target_count);
    }
    if (divisor < 2) divisor = 2;
    if (divisor > 65500) divisor = 65536;

    count = os_round_int((os_double)hw_clock_frequency / (os_double)(divisor * frequency_hz));

    os_memclear(&config, sizeof(config));
    config.alarm_en = TIMER_ALARM_EN;
    config.counter_en = TIMER_PAUSE;
    config.intr_type = TIMER_INTR_LEVEL;
    config.counter_dir = TIMER_COUNT_UP;
    config.auto_reload = OS_TRUE;
    config.divider = divisor;
#ifdef TIMER_GROUP_SUPPORTS_XTAL_CLOCK
    config.clk_src = TIMER_SRC_CLK_APB;
#endif
    timer_init(timer_group, timer_nr, &config);

    timer_set_counter_value(timer_group, timer_nr, 0);
    timer_set_alarm_value(timer_group, timer_nr, count);

    timer_isr_register(timer_group, timer_nr, prm->int_handler_func,
        (void*)(timer_group | (timer_nr << 4)), ESP_INTR_FLAG_IRAM, &s_timer_handle);

    /* Start listening for global interrupt enable functions.
       It is important to clear PIN_INTERRUPT_ENABLED for soft reboot.
     */
    pin_set_prm(pin, PIN_INTERRUPT_ENABLED, 0);
    enable = osal_add_interrupt_to_list(pin_timer_global_interrupt_control, (void*)pin);
    pin_timer_set_interrupt_enable_flag(pin, enable, PIN_GLOBAL_INTERRUPTS_ENABLED);
    pin_timer_set_interrupt_enable_flag(pin, OS_TRUE, PIN_INTERRUPTS_ENABLED_FOR_PIN);

    pin_timer_control_interrupt(pin);

#else
    hw_timer_t * t;
    os_int timer_nr, frequency_hz, divisor;
    const os_int hw_clock_frequency = 80000000; // 80 MHz

    timer_nr = pin_get_prm(pin, PIN_TIMER_SELECT);
    frequency_hz = pin_get_frequency(pin, 50);

    divisor = 1;
    if (frequency_hz >= 1)
    {
        divisor = hw_clock_frequency / frequency_hz;
    }
    if (divisor < 2) divisor = 2;

    t = timerBegin(timer_nr, 1, true);
    timerAttachInterrupt(t, prm->int_handler_func, true);
    timerAlarmWrite(t, divisor, true);
    timerAlarmEnable(t);
#endif
}


/**
****************************************************************************************************

  @brief Detach interrupt from timer.
  @anchor pin_timer_detach_interrupt

  The pin_timer_detach_interrupt() function detaches inperrupt, at minimum it disables
  the timer interrupt.

  @param   pin Pointer to the pin structure.
  @return  None.

****************************************************************************************************
*/
void pin_timer_detach_interrupt(
    const struct Pin *pin)
{
#ifdef ESP_PLATFORM
    pin_timer_set_interrupt_enable_flag(pin, OS_FALSE, PIN_INTERRUPTS_ENABLED_FOR_PIN);
    pin_timer_control_interrupt(pin);
#else
    detachInterrupt(pin->addr);
#endif
}


/**
****************************************************************************************************

  @brief Global enable/disable interrupts callback for flash writes.
  @anchor pin_timer_global_interrupt_control

  The pin_timer_global_interrupt_control() function is callback function from
  global interrupt control. The purpose of global control is to disable interrupts
  when writing to flash.

  @param   enable OS_TRUE to mark that interrupts are enabled globally, or OS_FALSE
           to mark that interrupts are disabled.
  @param   context Pointer to the pin structure, callback context.
  @return  None.

****************************************************************************************************
*/
static void pin_timer_global_interrupt_control(
    os_boolean enable,
    void *context)
{
    const struct Pin *pin;
    pin = (const struct Pin*)context;

    pin_timer_set_interrupt_enable_flag(pin, enable, PIN_GLOBAL_INTERRUPTS_ENABLED);
    pin_timer_control_interrupt(pin);
}


/**
****************************************************************************************************

  @brief Helper function to modify PIN_INTERRUPT_ENABLED bits.
  @anchor pin_timer_set_interrupt_enable_flag

  The pin_timer_set_interrupt_enable_flag() function is helper function to
  set or clear a bit in PIN_INTERRUPT_ENABLED parameter.

  Bit PIN_GLOBAL_INTERRUPTS_ENABLED indicates that interrupts are globally enabled
  and we are not now writing to flash.
  Bit PIN_INTERRUPTS_ENABLED_FOR_PIN indicates if interrupts are enabled specifically
  for this timer.

  @param   pin Pointer to timer's pin structure.
  @param   enable OS_TRUE to set the bit flag, OS_FALSE to clear it.
  @param   flag PIN_GLOBAL_INTERRUPTS_ENABLED or PIN_INTERRUPTS_ENABLED_FOR_PIN
  @return  None.

****************************************************************************************************
*/
static void pin_timer_set_interrupt_enable_flag(
   const struct Pin *pin,
   os_boolean enable,
   os_int flag)
{
   os_int x;

   x = pin_get_prm(pin, PIN_INTERRUPT_ENABLED);
   if (enable) {
       x |= flag;
   }
   else {
       x &= ~flag;
   }
   pin_set_prm(pin, PIN_INTERRUPT_ENABLED, x);
}


/**
****************************************************************************************************

  @brief Enable or disable interrups for the timer.
  @anchor pin_timer_control_interrupt

  The pin_timer_control_interrupt() function enables or disables interrupts
  according PIN_INTERRUPT_ENABLED parameter.

  @param   pin Pointer to timer's pin structure.
  @return  None.

****************************************************************************************************
*/
static void pin_timer_control_interrupt(
    const struct Pin *pin)
{
    os_int x, timer_group, timer_nr;
    timer_nr = pin_get_prm(pin, PIN_TIMER_SELECT);
    timer_group = pin_get_prm(pin, PIN_TIMER_GROUP_SELECT);
    x = pin_get_prm(pin, PIN_INTERRUPT_ENABLED);
    if ((x & (PIN_GLOBAL_INTERRUPTS_ENABLED|PIN_INTERRUPTS_ENABLED_FOR_PIN))
        == (PIN_GLOBAL_INTERRUPTS_ENABLED|PIN_INTERRUPTS_ENABLED_FOR_PIN))
    {
        if ((x & PIN_GPIO_PIN_INTERRUPTS_ENABLED) == 0)
        {
            timer_enable_intr(timer_group, timer_nr);
            x |= PIN_GPIO_PIN_INTERRUPTS_ENABLED;
            pin_set_prm(pin, PIN_INTERRUPT_ENABLED, x);
            timer_start(timer_group, timer_nr);
        }
    }
    else
    {
        if (x & PIN_GPIO_PIN_INTERRUPTS_ENABLED)
        {
            timer_pause(timer_group, timer_nr);
            timer_disable_intr(timer_group, timer_nr);
            x &= ~PIN_GPIO_PIN_INTERRUPTS_ENABLED;
            pin_set_prm(pin, PIN_INTERRUPT_ENABLED, x);
        }
    }
}
