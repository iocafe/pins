/**

  @file    arduino/pins_basics.c
  @brief   Pins library basic functionality.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    8.1.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include <Arduino.h>
#include "pins.h"

#ifdef ESP_PLATFORM
#include "driver/ledc.h"
#include "driver/periph_ctrl.h"
#endif

static void pin_ll_setup_pwm(
    const Pin *pin);

/**
****************************************************************************************************

  @brief Initialize Hardware IO.
  @anchor pins_ll_setup

  The pins_ll_setup() function...
  @param   pins_hdr Top level pins IO configuration structure.
  @param   flags Reserved for future, set 0 for now.
  @return  None.

****************************************************************************************************
*/
void pins_ll_setup(
    const IoPinsHdr *pins_hdr,
    os_int flags)
{
    const PinGroupHdr **group;
    const Pin *pin;
    os_short gcount, pcount;

    os_int
        is_touch_sensor;

    gcount = pins_hdr->n_groups;
    group =  pins_hdr->group;

periph_module_enable(PERIPH_LEDC_MODULE); // ?????????????????????????? THIS SHOULD BE COMMON FOR LIB, NOT FOR PIN GROUP

    while (gcount--)
    {
        pcount = (*group)->n_pins;
        pin = (*group)->pin;
        while (pcount--)
        {
            if (pin->addr >=  0) switch (pin->type)
            {
                case PIN_INPUT:
                    is_touch_sensor = pin_get_prm(pin, PIN_TOUCH);

                    if (!is_touch_sensor)
                    {
                        pinMode(pin->addr, pin_get_prm(pin, PIN_PULL_UP) ? INPUT_PULLUP : INPUT);
                    }
                    break;

                case PIN_OUTPUT:
                    pinMode(pin->addr, OUTPUT);
                    break;

                case PIN_PWM:
                    pin_ll_setup_pwm(pin);
                    break;

                case PIN_ANALOG_INPUT:
                case PIN_ANALOG_OUTPUT:
                case PIN_TIMER:
                    break;
            }

            pin++;
        }

        group++;
    }
}


/**
****************************************************************************************************

  @brief Setup a pin as PWM.
  @anchor pin_ll_setup_pwm

  ESP32 note: Generate 1 MHz clock signal with ESP32,  note 24.3.2020/pekka
    LEDC peripheral can be used to generate clock signals between
       40 MHz (half of APB clock) and approximately 0.001 Hz.
       Please check the LEDC chapter in Technical Reference Manual.


  The pin_ll_setup_pwm() function...
  @return  None.

****************************************************************************************************
*/
static void pin_ll_setup_pwm(
    const Pin *pin)
{
#ifdef ESP_PLATFORM
    os_int
        frequency_hz,
        resolution_bits,
        initial_state,
        timer_nr;

    ledc_timer_config_t ledc_timer;
    ledc_channel_config_t channel_config;

    frequency_hz = pin_get_prm(pin, PIN_FREQENCY);
    if (!frequency_hz)
    {
        /* Should be 1024, not 1000: keeps it precise */
        frequency_hz = 1024 * pin_get_prm(pin, PIN_FREQENCY_KHZ);
        if (!frequency_hz)
        {
            frequency_hz = 50; /* Default servo frequency */
        }
    }
    resolution_bits = pin_get_prm(pin, PIN_RESOLUTION);
    if (!resolution_bits) resolution_bits = 12;
    initial_state = pin_get_prm(pin, PIN_INIT);
    timer_nr = pin_get_prm(pin, PIN_TIMER_SELECT);

    /* Set up timer
     */
    os_memclear(&ledc_timer, sizeof(ledc_timer));
    ledc_timer.duty_resolution = resolution_bits,
    ledc_timer.freq_hz = frequency_hz,
    ledc_timer.speed_mode = LEDC_HIGH_SPEED_MODE;
    ledc_timer.timer_num = timer_nr;  // LEDC_TIMER_0

    /* I think not needed for new esp-idf software, try uncommenting
       ledc_timer.clk_cfg = LEDC_USE_APB_CLK */
    ledc_timer_config(&ledc_timer); // Set up GPIO PIN

    os_memclear(&channel_config, sizeof(channel_config));
    channel_config.channel    = pin->bank; // LEDC_CHANNEL_0
    channel_config.duty       = initial_state;
    channel_config.gpio_num   = pin->addr;
    channel_config.speed_mode = LEDC_HIGH_SPEED_MODE;
    channel_config.timer_sel  = timer_nr; // LEDC_TIMER_0
    channel_config.hpoint = 500 * pin->bank;
    ledc_channel_config(&channel_config);

    /* ledcSetup(pin->bank, frequency_hz, resolution_bits);
    ledcAttachPin(pin->addr, pin->bank);
    ledcWrite(pin->bank, initial_state); */
#endif
}


/**
****************************************************************************************************

  @brief Set IO pin state.
  @anchor pin_ll_set

  The pin_ll_set() function...
  @return  None.

****************************************************************************************************
*/
void pin_ll_set(
    const Pin *pin,
    os_int x)
{
    if (pin->addr >= 0) switch (pin->type)
    {
        case PIN_OUTPUT:
            digitalWrite(pin->addr, x);
            break;

        case PIN_PWM:
            ledcWrite(pin->bank, x);
            break;

    case PIN_ANALOG_OUTPUT:
#ifdef ESP_PLATFORM
            dacWrite(pin->bank, x);
            break;
#endif

        case PIN_INPUT:
        case PIN_ANALOG_INPUT:
        case PIN_TIMER:
            break;
    }
}


/**
****************************************************************************************************

  @brief Get IO pin state.
  @anchor pin_get

  The pin_ll_get() function...
  @return  None.

****************************************************************************************************
*/
os_int pin_ll_get(
    const Pin *pin)
{
    if (pin->addr >= 0) switch (pin->type)
    {
        case PIN_INPUT:
            /* Touch sensor
             */
            if (pin->prm) {
                if (pin_get_prm(pin, PIN_TOUCH)) {
                    return touchRead(pin->addr);
                }
            }

            /* Normal digital input
             */
            return digitalRead(pin->addr);

        case PIN_ANALOG_INPUT:
            return analogRead(pin->addr);

        case PIN_OUTPUT:
        case PIN_PWM:
        case PIN_ANALOG_OUTPUT:
        case PIN_TIMER:
            break;
    }

    return 0;
}

