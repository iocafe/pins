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

static void pin_ll_setup_input(
    const Pin *pin);

static void pin_ll_setup_output(
    const Pin *pin);

static void pin_ll_setup_pwm(
    const Pin *pin);


/**
****************************************************************************************************

  @brief Initialize hardware IO library.
  @anchor pins_ll_initialize

  @return  None.

****************************************************************************************************
*/
void pins_ll_initialize(
    void)
{
    periph_module_enable(PERIPH_LEDC_MODULE);
    gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
}


/**
****************************************************************************************************

  @brief Initialize hardware IO pin.
  @anchor pin_ll_setup

  The pin_ll_setup() function...
  @param   pin Pin to initialize.
  @param   flags Reserved for future, set 0 for now.
  @return  None.

****************************************************************************************************
*/
void pin_ll_setup(
    const Pin *pin,
    os_int flags)
{
    os_int
        is_touch_sensor;

    if (pin->addr >=  0) switch (pin->type)
    {
        case PIN_INPUT:
            is_touch_sensor = pin_get_prm(pin, PIN_TOUCH);
            if (!is_touch_sensor)
            {
                pin_ll_setup_input(pin);
            }
            break;

        case PIN_OUTPUT:
            pin_ll_setup_output(pin);
            break;

        case PIN_PWM:
            pin_ll_setup_pwm(pin);
            break;

        case PIN_ANALOG_INPUT:
        case PIN_ANALOG_OUTPUT:
        case PIN_TIMER:
            break;
    }
}


/**
****************************************************************************************************

  @brief Setup a pin as input.
  @anchor pin_ll_setup_input

  The pin_ll_setup_input() function...
  @return  None.

****************************************************************************************************
*/
static void pin_ll_setup_input(
    const Pin *pin)
{
#ifdef ESP_PLATFORM
    gpio_config_t io_conf;
    os_memclear(&io_conf, sizeof(io_conf));

    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = 1ULL << pin->addr;
    io_conf.pull_down_en = pin_get_prm(pin, PIN_PULL_DOWN);
    io_conf.pull_up_en = pin_get_prm(pin, PIN_PULL_UP);
    gpio_config(&io_conf);
#else
    in mode = INPUT;
    if (pin_get_prm(pin, PIN_PULL_UP)) mode = INPUT_PULLUP;
    if (pin_get_prm(pin, PIN_PULL_DOWN)) mode = INPUT_PULLDOWN;
    pinMode(pin->addr, mode);
#endif
}


/**
****************************************************************************************************

  @brief Setup a pin as output.
  @anchor pin_ll_setup_output

  The pin_ll_setup_output() function...
  @return  None.

****************************************************************************************************
*/
static void pin_ll_setup_output(
    const Pin *pin)
{
#ifdef ESP_PLATFORM
    gpio_config_t io_conf;
    os_memclear(&io_conf, sizeof(io_conf));

    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = 1ULL << pin->addr;
    gpio_config(&io_conf);
#else
    pinMode(pin->addr, OUTPUT);
#endif
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
    ledc_timer.timer_num = timer_nr;  // LEDC_TIMER_0

    /* I think not needed for new esp-idf software, try uncommenting
       ledc_timer.clk_cfg = LEDC_USE_APB_CLK */
    ledc_timer_config(&ledc_timer); // Set up GPIO PIN

    os_memclear(&channel_config, sizeof(channel_config));
    channel_config.channel    = pin->bank; // LEDC_CHANNEL_0
    channel_config.duty       = initial_duty;
    channel_config.gpio_num   = pin->addr;
    channel_config.speed_mode = LEDC_HIGH_SPEED_MODE;
    channel_config.timer_sel  = timer_nr; // LEDC_TIMER_0
    channel_config.hpoint = hpoint;
    ledc_channel_config(&channel_config);
#else
    /* ledcSetup(pin->bank, frequency_hz, resolution_bits);
    ledcAttachPin(pin->addr, pin->bank);
    ledcWrite(pin->bank, initial_duty); */
#endif
}

