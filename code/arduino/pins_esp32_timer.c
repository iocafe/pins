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

TaskHandle_t complexHandlerTask;
hw_timer_t * adcTimer = NULL; // our timer

void pin_setup_timer(
    const struct Pin *pin,
    pinTimerParams *prm)
{
    adcTimer = timerBegin(3, 80, true); // 80 MHz / 80 = 1 MHz hardware clock for easy figuring
    timerAttachInterrupt(adcTimer, prm->int_handler_func, true); // Attaches the handler function to the timer
    timerAlarmWrite(adcTimer, 4, true);
    timerAlarmEnable(adcTimer);
}
