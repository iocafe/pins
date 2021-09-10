/**

  @file    duino/pins_duino_pwm.c
  @brief   ESP32 pulse width modulation.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    26.4.2021

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "pins.h"
#ifdef OSAL_ARDUINO

/**
****************************************************************************************************

  @brief Configure a pin as PWM.
  @anchor pin_pwm_setup

  The pin_pwm_setup() function...

  ESP32 note: Generate 1 MHz clock signal with ESP32,  note 24.3.2020/pekka
    LEDC peripheral can be used to generate clock signals between
       40 MHz (half of APB clock) and approximately 0.001 Hz.
       Please check the LEDC chapter in Technical Reference Manual.

  @param   pin Pointer to the pin structure.
  @return  None.

****************************************************************************************************
*/
void pin_pwm_setup(
    const Pin *pin)
{
    /* ledcSetup(pin->bank, frequency_hz, resolution_bits);
    ledcAttachPin(pin->addr, pin->bank);
    ledcWrite(pin->bank, initial_duty); */
}

#endif
