/**

  @file    esp32/pins_esp32_analog.c
  @brief   ADC & DAC.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    26.4.2021

  The ESP32 integrates two 12-bit SAR (Successive Approximation Register) ADCs, supporting a 
  total of 18 measurement channels (analog enabled pins).

  ADC1:
        8 channels: GPIO32 - GPIO39

  ADC2:
        10 channels: GPIO0, GPIO2, GPIO4, GPIO12 - GPIO15, GPIO25 - GPIO27

  Without attenuation, ESP analog input range is from 0 to 800 mV. Setting attenuation 
  increases signal range. For example 11db attenuation will result signal range from 0 to 2.84V 

    ADC_ATTEN_DB_0      No input attenuation, ADC can measure up to approx. 800 mV.
    ADC_ATTEN_DB_2_5    Extends the range about 2.5 dB (1.33 x)
    ADC_ATTEN_DB_6      Extends the range about 6 dB (2 x)
    ADC_ATTEN_DB_11     Extends the range about 11 dB (3.55 x)

  ESP32-S2 has two 8-bit DAC (digital to analog converter) channels, connected to GPIO17 
  (Channel 1) and GPIO18 (Channel 2).

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "pins.h"
#ifdef OSAL_ESP32
#include "driver/adc.h"
#include "driver/dac.h"
#include "code/esp32/pins_esp32_analog.h"

#define PIN_ADC1 0x40
#define PIN_ADC2 0x80
#define PIN_ADC_CH_MASK 0x3F

static os_uchar pin_adc_map[] =
{
    ADC2_CHANNEL_0 | PIN_ADC2,  /* GPIO 0 */
    0,                          /* 1 */
    ADC2_CHANNEL_1 | PIN_ADC2,  /* GPIO 2 */
    0,                          /* 3 */
    ADC2_CHANNEL_2 | PIN_ADC2,  /* GPIO 4 */
    0,                          /* 5 */
    0,                          /* 6 */
    0,                          /* 7 */
    0,                          /* 8 */
    0,                          /* 9 */
    0,                          /* 10 */
    0,                          /* 11 */
    ADC2_CHANNEL_3 | PIN_ADC2,  /* GPIO 12 */
    ADC2_CHANNEL_4 | PIN_ADC2,  /* GPIO 13 */
    ADC2_CHANNEL_5 | PIN_ADC2,  /* GPIO 14 */
    ADC2_CHANNEL_6 | PIN_ADC2,  /* GPIO 15 */
    0,                          /* 16 */
    0,                          /* 17 */
    0,                          /* 18 */
    0,                          /* 19 */
    0,                          /* 20 */
    0,                          /* 21 */
    0,                          /* 22 */
    0,                          /* 23 */
    0,                          /* 24 */
    ADC2_CHANNEL_7 | PIN_ADC2,  /* GPIO 25 */
    ADC2_CHANNEL_8 | PIN_ADC2,  /* GPIO 26 */
    ADC2_CHANNEL_9 | PIN_ADC2,  /* GPIO 27 */
    0,                          /* 28 */
    0,                          /* 29 */
    0,                          /* 30 */
    0,                          /* 31 */
    ADC1_CHANNEL_0 | PIN_ADC1,  /* GPIO 32 */
    ADC1_CHANNEL_1 | PIN_ADC1,  /* GPIO 33 */
    ADC1_CHANNEL_2 | PIN_ADC1,  /* GPIO 34 */
    ADC1_CHANNEL_3 | PIN_ADC1,  /* GPIO 35 */
    ADC1_CHANNEL_4 | PIN_ADC1,  /* GPIO 36 */
    ADC1_CHANNEL_5 | PIN_ADC1,  /* GPIO 37 */
    ADC1_CHANNEL_6 | PIN_ADC1,  /* GPIO 38 */
    ADC1_CHANNEL_7 | PIN_ADC1,  /* GPIO 39 */
};

#define PIN_ADC_MAP_LEN (sizeof(pin_adc_map)/sizeof(os_uchar))


/**
****************************************************************************************************

  @brief  Configure a pin as analog input.
  @anchor pin_setup_analog_input

  Configures a GPIO pin as analog input.
  - ADC1, 8 channels: GPIO32 - GPIO39
  - ADC2, 10 channels: GPIO0, GPIO2, GPIO4, GPIO12 - GPIO15, GPIO25 - GPIO27

  @param   pin Pointer to pin structure.

****************************************************************************************************
*/
void pin_setup_analog_input(
    const Pin *pin)
{
    os_int addr;
    os_uchar c;

    addr = pin->addr;
    if (addr >= 0 && addr < PIN_ADC_MAP_LEN) 
    {
        c = pin_adc_map[addr];
        if (c & PIN_ADC1) {
            adc1_config_width(ADC_WIDTH_BIT_12);
            adc1_config_channel_atten(c & PIN_ADC_CH_MASK, ADC_ATTEN_DB_11);    
            return;
        }
        if (c & PIN_ADC2) {
            adc2_config_channel_atten(c & PIN_ADC_CH_MASK, ADC_ATTEN_DB_11);
            return;
        }
    }

    osal_debug_error_int("pin cannot be used as analog input, gpio=", addr);
}


/**
****************************************************************************************************

  @brief Read analog input.
  @anchor pin_read_analog_input

  Reads single value from analog inputs. ESP32 analog inputs are 12 bit, thus range from 0 to 4095.
  
  @param   pin Pointer to pin structure.
  @param   state_bits Pointer to byte where to store state bits like OSAL_STATE_CONNECTED,
           OSAL_STATE_ORANGE, OSAL_STATE_YELLOW... Value OSAL_STATE_UNCONNECTED indicates not
           connected (= unknown value).
  @return  Pin value, for example 0 or 1 for digital input.

****************************************************************************************************
*/
os_int OS_ISR_FUNC_ATTR pin_read_analog_input(
    const Pin *pin,
    os_char *state_bits)
{
    os_int addr;
    os_uchar c;
    int read_raw;
    esp_err_t rval;

    addr = pin->addr;
    if (addr >= 0 && addr < PIN_ADC_MAP_LEN) 
    {
        c = pin_adc_map[addr];
        if (c & PIN_ADC1) 
        {
            *state_bits = OSAL_STATE_CONNECTED;
            return adc1_get_raw(c & PIN_ADC_CH_MASK);
        }

        if (c & PIN_ADC2) 
        {
            rval = adc2_get_raw(c & PIN_ADC_CH_MASK, ADC_WIDTH_BIT_12, &read_raw);
            if (rval == ESP_OK) {
                *state_bits = OSAL_STATE_CONNECTED;
                return read_raw;
            }
            if (rval == ESP_ERR_TIMEOUT ) {
                /* ADC2 used by Wi-Fi */
                *state_bits = OSAL_STATE_CONNECTED|OSAL_STATE_ORANGE;
                return 0;
            }
        }
    }

    *state_bits = OSAL_STATE_UNCONNECTED|OSAL_STATE_RED;
    return 0;
}


/**
****************************************************************************************************

  @brief Configure a pin as analog output.
  @anchor pin_setup_analog_output

  Configures a GPIO pin as analog output. On ESP32 pins 17 and 18 are GPIO pins which can be 
  used as analog outputs. 

  @param   pin Pointer to pin structure.

****************************************************************************************************
*/
void pin_setup_analog_output(
    const Pin *pin)
{
    int c;

    switch (pin->addr)
    {
        case 17: c = DAC_CHANNEL_1; break;
        case 18: c = DAC_CHANNEL_2; break;
        default: 
            osal_debug_error_int("pin cannot be used as analog output, gpio=", pin->addr);
            return;
    }

    dac_output_enable(c);
}


/**
****************************************************************************************************

  @brief Set analog output.
  @anchor pin_write_analog_output

  Write value 0 - 255 to analog output pin. On ESP32 pins 17 and 18 are GPIO pins which can be 
  used as analog outputs. 

  @param   pin Pointer to pin structure.
  @param   x Value from 0 to 255 to set (8 bit DAC).

****************************************************************************************************
*/
void OS_ISR_FUNC_ATTR pin_write_analog_output(
    const Pin *pin,
    os_int x)
{
    int c;

    switch (pin->addr)
    {
        case 17: c = DAC_CHANNEL_1; break;
        case 18: c = DAC_CHANNEL_2; break;
        default: return;
    }
    dac_output_voltage(c, x);
}

#endif