/**

  @file    esp32/pins_esp32_pwm.c
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
#ifdef OSAL_ESP32
#include "driver/ledc.h"
#include "code/esp32/pins_esp32_pwm.h"

/**
****************************************************************************************************

  @brief Configure a pin as PWM.
  @anchor pin_pwm_setup

  The pin_pwm_setup() function...

  ESP32 note: Generate 1 MHz clock signal with ESP32,  note 24.3.2020/pekka
    LEDC peripheral can be used to generate clock signals between
       40 MHz (half of APB clock) and approximately 0.001 Hz.
       Please check the LEDC chapter in Technical Reference Manual.

       "timer": 0 - 3 selects timer to use. All PWM outputs sharing same timer share same frequency.
       I think timer 0 and channel 0 are used for esp32 camera, if connected?

       "bank": 0 - 7 select the channel. It needs to be own value for each PWM output. Same bank
       number cannot be reused with different channels.

  @param   pin Pointer to the pin structure.
  @return  None.

****************************************************************************************************
*/
void pin_pwm_setup(
    const Pin *pin)
{
    os_int
        frequency_hz,
        resolution_bits,
        initial_duty,
        hpoint,
        timer_nr;

    ledc_timer_config_t ledc_timer;
    ledc_channel_config_t channel_config;

    /* 50 = default servo frequency.
     */
    frequency_hz = pin_get_frequency(pin, 50);
    resolution_bits = pin_get_prm(pin, PIN_RESOLUTION);
    if (!resolution_bits) resolution_bits = 12;
    initial_duty = pin_get_prm(pin, PIN_INIT);
    hpoint = pin_get_prm(pin, PIN_HPOINT);
    timer_nr = pin_get_prm(pin, PIN_TIMER_SELECT);

    /* Set up timer
     */
    os_memclear(&ledc_timer, sizeof(ledc_timer));
    ledc_timer.duty_resolution = resolution_bits,
    ledc_timer.freq_hz = frequency_hz,
    ledc_timer.speed_mode = LEDC_HIGH_SPEED_MODE;
    ledc_timer.timer_num = timer_nr;  // 0 = LEDC_TIMER_0, ...

    /* I think needed for new esp-idf software ?
     */
    ledc_timer_config(&ledc_timer); // Set up GPIO PIN

    os_memclear(&channel_config, sizeof(channel_config));
    channel_config.channel    = pin->bank; // LEDC_CHANNEL_0
    channel_config.duty       = initial_duty;
    channel_config.gpio_num   = pin->addr;
    channel_config.speed_mode = LEDC_HIGH_SPEED_MODE;
    channel_config.timer_sel  = timer_nr; // LEDC_TIMER_0
    channel_config.hpoint = hpoint;
    #if IDF_VERSION_MAJOR >= 4
       ledc_timer.clk_cfg = LEDC_USE_APB_CLK;
     #endif
    ledc_channel_config(&channel_config);
}


/**
****************************************************************************************************

  @brief Set PWM duty cycle.
  @anchor pin_pwm_set

  Note, not thread safe: PWM channel duty must be modified only from one thread at a time. 
  If multiple threads modify PWM simultaneously, resulting duty cycle can be unexpected.

  @param   pin Pointer to pin structure.
  @param   x Value to set, for example 0 or 1 for digital output.

****************************************************************************************************
*/
void OS_ISR_FUNC_ATTR pin_pwm_set(
    const Pin *pin,
    os_int x)
{
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, pin->bank, (uint32_t)x);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, pin->bank);
}

#endif
