/**

  @file    simulation/pins_basics.c
  @brief   Pins library basic functionality.
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

    gcount = pins_hdr->n_groups;
    group = pins_hdr->group;
    while (gcount--)
    {
        pcount = (*group)->n_pins;
        pin = (*group)->pin;
        while (pcount--)
        {

            pin++;
        }


        group++;
    }
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
}


/**
****************************************************************************************************

  @brief Get IO pin state.
  @anchor pin_ll_get

  The pin_ll_get() function...
  @return  None.

****************************************************************************************************
*/
os_int pin_ll_get(
    const Pin *pin)
{
    return 0;
}
