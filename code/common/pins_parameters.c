/**

  @file    common/pins_parameters.c
  @brief   Run time access to IO pin parameters.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    16.9.2019

  Copyright 2012 - 2019 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
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
    os_short *p;
    os_char count;

    p = pin->prm + PINS_N_RESERVED;
    count = (pin->prm_n - PINS_N_RESERVED) / 2;
    while (count-- > 0)
    {
        if (p[0] == prm)
        {
            p[1] = (os_short)value;
            break;
        }
        p += 2;
    }
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
    os_short *p;
    os_char count;

    p = pin->prm + PINS_N_RESERVED;
    count = (pin->prm_n - PINS_N_RESERVED) / 2;
    while (count-- > 0)
    {
        if (p[0] == prm)
        {
            return p[1];
        }
        p += 2;
    }
    return 0;
}
