/**

  @file    arduino/pins_esp32_timer.c
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
#include "Arduino.h"


void pin_setup_timer(
    const struct Pin *pin,
    pinTimerParams *prm)
{
    hw_timer_t * t;
    os_int timer_nr, frequency_hz, divisor;
    const os_int hw_clock_frequency = 40000000; // 80 MHz

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
    if (pin_get_prm(pin, PIN_INIT))
    {
        timerAlarmEnable(t);
    }
}
