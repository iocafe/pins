/**

  @file    esp32/pins_esp32_timer.c
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
// #include "Arduino.h"

// #include "esp_intr_alloc.h"
 #include "esp_attr.h"
#include "driver/timer.h"

typedef void pins_esp_timer_int_handle(void *);

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


void pin_timer_attach_interrupt(
    const struct Pin *pin,
    pinTimerParams *prm)
{
return;
#ifdef ESP_PLATFORM
    timer_config_t config;
    os_int timer_nr, timer_group, frequency_hz, divisor, target_count, count;
    const os_int hw_clock_frequency = 80000000; // 80 MHz
    os_boolean enable;

    intr_handle_t s_timer_handle;

    timer_nr = pin_get_prm(pin, PIN_TIMER_SELECT);
    timer_group = pin_get_prm(pin, PIN_TIMER_GROUP_SELECT);
    frequency_hz = pin_get_frequency(pin, 50);
    target_count = 100000;

    divisor = 1;
    if (frequency_hz >= 1)
    {
        divisor = hw_clock_frequency / (frequency_hz * target_count);
    }
    if (divisor < 2) divisor = 2;
    if (divisor > 65500) divisor = 65536;

    count = os_round_int((os_double)hw_clock_frequency / (os_double)(divisor * frequency_hz));

    os_memclear(&config, sizeof(config));
    config.alarm_en = OS_TRUE;				//Alarm Enable
    config.counter_en = OS_FALSE;			//If the counter is enabled it will start incrementing / decrementing immediately after calling timer_init()
    config.intr_type = TIMER_INTR_LEVEL;	//Is interrupt is triggered on timer’s alarm (timer_intr_mode_t)
    config.counter_dir = TIMER_COUNT_UP;	//Does counter increment or decrement (timer_count_dir_t)
    config.auto_reload = OS_TRUE;           //If counter should auto_reload a specific initial value on the timer’s alarm, or continue incrementing or decrementing.
    config.divider = divisor;                    //Divisor of the incoming 80 MHz (12.5nS) APB_CLK clock. E.g. 80 = 1uS per timer tick

    timer_init(timer_group, timer_nr, &config);
    timer_set_counter_value(timer_group, timer_nr, 0);
    timer_set_alarm_value(timer_group, timer_nr, count);

    timer_isr_register(timer_group, timer_nr, (pins_esp_timer_int_handle*)(prm->int_handler_func), NULL, 0, &s_timer_handle);
    timer_start(timer_group, timer_nr);

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
    // const os_int hw_clock_frequency = 40000000; // 80 MHz

    timer_nr = pin_get_prm(pin, PIN_TIMER_SELECT);
    frequency_hz = pin_get_frequency(pin, 50);

    divisor = 1;
    if (frequency_hz >= 1)
    {
        divisor = hw_clock_frequency / frequency_hz;
    }
    if (divisor < 1) divisor = 1;

    t = timerBegin(timer_nr, 1, true);
    timerAttachInterrupt(t, prm->int_handler_func, true);
    timerAlarmWrite(t, divisor, true);
    timerAlarmEnable(t);
#endif
}


void pin_timer_detach_interrupt(
    const struct Pin *pin)
{
    return;
#ifdef ESP_PLATFORM
    pin_timer_set_interrupt_enable_flag(pin, OS_FALSE, PIN_INTERRUPTS_ENABLED_FOR_PIN);
    pin_timer_control_interrupt(pin);
#else
    detachInterrupt(pin->addr);
#endif
}


/* Global enable/disable interrupts callback for flash writes.
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

/*
@param  flag PIN_GLOBAL_INTERRUPTS_ENABLED or PIN_INTERRUPTS_ENABLED_FOR_PIN
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
        }
    }
    else
    {
        if (x & PIN_GPIO_PIN_INTERRUPTS_ENABLED)
        {
            timer_disable_intr(timer_group, timer_nr);
            x &= ~PIN_GPIO_PIN_INTERRUPTS_ENABLED;
            pin_set_prm(pin, PIN_INTERRUPT_ENABLED, x);
        }
    }
}
