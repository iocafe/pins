/**

  @file    common/pins_parameters.c
  @brief   Run time access to IO pin parameters.
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

/**
****************************************************************************************************

  @brief Modify value of an IO pin parameter.
  @anchor pin_set_prm

  The pin_set_prm() function sets parameter value. Notce that only parameters configured for
  the IO pin in JSON can be set. Attempt to set not configured parameter, will be ignored.

  @param   pin Pointer to static information structure for the pin.
  @param   prm Parameter number which to get like PIN_TOUCH, PIN_SPEED... See pinPrm enumeration
           in pins_basics.h for full list.

  @return  None

****************************************************************************************************
*/
void pin_set_prm(
    const Pin *pin,
    pinPrm prm,
    os_int value)
{
    PinPrmValue *p;
    os_char count;

    p = pin->prm + PINS_N_RESERVED;
    count = pin->prm_n - PINS_N_RESERVED;
    while (count-- > 0)
    {
        if (p->ix == prm) {
            p->value = (os_short)value;
            return;
        }
        p++;
    }

    osal_debug_error_int("Attemp to set nonexistent pin parameter ", prm);
}


/**
****************************************************************************************************

  @brief Get value of an IO pin parameter.
  @anchor pin_get_prm

  The pin_get_prm() function returns a parameter value configured for the IO pin in JSON. Some
  parameter values may also be modified in run time.

  @param   pin Pointer to static information structure for the pin.
  @param   prm Parameter number which to get like PIN_TOUCH, PIN_SPEED... See pinPrm enumeration
           in pins_basics.h for full list.

  @return  Current value of the parameter. The function returns 0 if parameter is not specified.

****************************************************************************************************
*/
os_int pin_get_prm(
    const Pin *pin,
    pinPrm prm)
{
    PinPrmValue *p;
    os_char count;

    p = pin->prm + PINS_N_RESERVED;
    count = (pin->prm_n - PINS_N_RESERVED) / 2;
    while (count-- > 0)
    {
        if (p->ix == prm) {
            return p->value;
        }
        p++;
    }

    /* Default is zero, getting here is not error.
     */
    return 0;
}


/**
****************************************************************************************************

  @brief Get fequency setting for the pin.
  @anchor pin_get_frequency

  The pin_get_frequency() function returns a frequency setting for pin. Frequency can be
  specified as Hz or kHz, thus the function.

  @param   pin Pointer to static information structure for the pin.
  @param   default_frequency Default frequency to return if frequency is unspecified.

  @return  Frequency setting, if no frequency specified, the function returns the default
           frequency given as argument.

****************************************************************************************************
*/
os_int pin_get_frequency(
    const Pin *pin,
    os_int default_frequency)
{
    os_int frequency_hz;

    frequency_hz = pin_get_prm(pin, PIN_FREQENCY);
    if (!frequency_hz)
    {
        frequency_hz = 1000 * pin_get_prm(pin, PIN_FREQENCY_KHZ);
        if (!frequency_hz)
        {
            frequency_hz = 1000000 * pin_get_prm(pin, PIN_FREQENCY_MHZ);
            if (!frequency_hz)
            {
                frequency_hz = default_frequency;
            }
        }
    }
    return frequency_hz;
}


/**
****************************************************************************************************

  @brief Get speed setting for the pin.
  @anchor pin_get_speed

  The pin_get_speed() function returns a speed setting for pin. Frequency can be
  specified as bps or kbps, thus the function.

  @param   pin Pointer to static information structure for the pin.
  @param   default_speed Default speed to return if frequency is unspecified, bps.

  @return  Speed setting, if no speed is specified in JSON, the function returns the default
           speed given as argument.

****************************************************************************************************
*/
os_int pin_get_speed(
    const Pin *pin,
    os_int default_speed)
{
    os_uint speed;

    speed = pin_get_prm(pin, PIN_SPEED);
    if (!speed)
    {
        speed = 1000 * pin_get_prm(pin, PIN_SPEED_KBPS);
        if (!speed)
        {
            return default_speed;
        }
    }

    /* 100, scaling because speed is scaled to 16 bits by JSON.
     */
    return speed * 100;
}
