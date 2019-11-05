/**

  @file    common/pins_state.c
  @brief   Set and get pin states.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    3.11.2019

  Copyright 2012 - 2019 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "pinsx.h"

/* Function pointer to move a pin value to iocom. OS_NULL if not connected to IOCOM.
 */
pin_to_iocom_t *pin_to_iocom_func = OS_NULL;


/**
****************************************************************************************************

  @brief Initialize Hardware IO.
  @anchor pins_setup

  The pins_setup() function...
  @param   pins_hdr Top level pins IO configuration structure.
  @param   flags Reserved for future, set 0 for now.
  @return  None.

****************************************************************************************************
*/
void pins_setup(
    const IoPinsHdr *pins_hdr,
    os_int flags)
{
    pins_ll_setup(pins_hdr, flags);
}

/* Set IO pin state.
 */
void pin_set(
    const Pin *pin,
    os_int x)
{
    pin_ll_set(pin, x);
    if (x != *(os_int*)pin->prm)
    {
        *(os_int*)pin->prm = x;

        if (pin_to_iocom_func &&
            pin->signal)
        {
            pin_to_iocom_func(pin, x);
        }
    }
}

/* Get pin state.
 */
os_int pin_get(
    const Pin *pin)
{
    os_int x;

    x = pin_ll_get(pin);
    if (x != *(os_int*)pin->prm)
    {
        *(os_int*)pin->prm = x;

        if (pin_to_iocom_func &&
            pin->signal)
        {
            pin_to_iocom_func(pin, x);
        }
    }
    return x;
}

/* Get pin state from memory.
 */
os_int pin_value(
    const Pin *pin)
{
    return *(os_int*)pin->prm;
}


/* Read all inputs of the IO device into global Pin structurees
 */
void pins_read_all(
    const IoPinsHdr *hdr,
    os_ushort flags)
{
    const PinGroupHdr *group;
    const Pin *pin;
    os_int x;
    os_short n_groups, n_pins, i, j;
    os_char type;

    n_groups = hdr->n_groups;

    for (i = 0; i<n_groups; i++)
    {
        group = hdr->group[i];
        pin = group->pin;
        type = pin->type;

        if (type != PIN_INPUT &&
            type != PIN_ANALOG_INPUT &&
            (flags & PINS_RESET_IOCOM) == 0)
        {
            continue;
        }

        n_pins = group->n_pins;

        for (j = 0; j < n_pins; j++, pin++)
        {
            x = pin_get(pin);
            if (x != *(os_int*)pin->prm || (flags & PINS_RESET_IOCOM))
            {
                *(os_int*)pin->prm = x;

                /* If this is PINS library is connected to IOCOM library
                   and this pin is mapped to IOCOM signal, then forward
                   the change to IOCOM.
                 */
                if (pin_to_iocom_func &&
                    pin->signal)
                {
                    pin_to_iocom_func(pin, x);
                }
            }
        }
    }
}

