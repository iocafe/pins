/**

  @file    common/pins_state.c
  @brief   Set and get pin states.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    21.4.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
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
    const PinGroupHdr * const *group;
    const Pin *pin;
    os_short gcount, pcount;

    pins_ll_initialize_lib();

    gcount = pins_hdr->n_groups;
    group = pins_hdr->group;

    while (gcount--)
    {
        pcount = (*group)->n_pins;
        pin = (*group)->pin;

        while (pcount--) {
            pin_ll_setup(pin++, flags);
        }

        group++;
    }

#if PINS_SPI || PINS_I2C
    /* SPI and I2C initialization. */
    pins_initialize_bus_devices();
#endif
}


#if OSAL_PROCESS_CLEANUP_SUPPORT
/**
****************************************************************************************************

  @brief Shut down the hardware IO.
  @anchor pins_setup

  The pins_shutdown() function is called before program exits to stop io devices and release
  resources for them. For example: Windows USB camera needs to be shut down.

  @param   pins_hdr Top level pins IO configuration structure.
  @return  None.

****************************************************************************************************
*/
void pins_shutdown(
    const IoPinsHdr *pins_hdr)
{
    const PinGroupHdr * const *group;
    const Pin *pin;
    os_short gcount, pcount;

    gcount = pins_hdr->n_groups;
    group = pins_hdr->group;

    while (gcount--)
    {
        pcount = (*group)->n_pins;
        pin = (*group)->pin;

        while (pcount--) {
            pin_ll_shutdown(pin++);
        }

        group++;
    }

    pins_ll_shutdown_lib();
}
#endif


/**
****************************************************************************************************

  @brief Set IO pin state.
  @anchor pin_set

  The pin_set() function writes pin value to IO hardware, stores it for the Pin structure and,
  if appropriate, writes pin value as IOCOM signal.

  @param   pin Pointer to pin configuration structure.
  @return  None.

****************************************************************************************************
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
            pin_to_iocom_func(pin);
        }
    }
}


/**
****************************************************************************************************

  @brief Get pin state.
  @anchor pin_get

  The pin_get() function reads pin value to IO hardware, stores it for the Pin structure and,
  if appropriate, writes the pin value as IOCOM signal.

  @param   pin Pointer to pin configuration structure.
  @return  Pin value from IO hardware.

****************************************************************************************************
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
            pin_to_iocom_func(pin);
        }
    }
    return x;
}


/**
****************************************************************************************************

  @brief Get pin state stored for the Pin structure.
  @anchor pin_get

  The pin_value() function returns pin value, which is stored for the Pin structure. It doesn't
  read hardware IO, nor deal with IOCOM.

  This function is useful if IO device reads all it's inputs ar beginning of loop() function,
  and values which are already in pin structure need to be accessed.

  @param   pin Pointer to pin configuration structure.
  @return  Memorized pin value.

****************************************************************************************************
*/
os_int pin_value(
    const Pin *pin)
{
    return *(os_int*)pin->prm;
}


/**
****************************************************************************************************

  @brief Read all inputs of the IO device into global Pin structures
  @anchor pins_read_all

  The pins_read_all() can be called at beginning of loop() function to read all hardware IO
  pins to memory and forward these as IO com signals as appropriate.

  The function is also used to set up initial state when connecting PINS library to IOCOM library.

  @param   hdr Pointer to IO hardware configuration structure.
  @param   PINS_DEFAULT to read all inputs in loop() function. PINS_RESET_IOCOM to set up
           initial state when connecting PINS library to IOCOM library.
  @return  None.

****************************************************************************************************
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
#if PINS_SIMULATED_INTERRUPTS
            type != PIN_TIMER &&
#endif
            (flags & PINS_RESET_IOCOM) == 0)
        {
            continue;
        }

        n_pins = group->n_pins;

        for (j = 0; j < n_pins; j++, pin++)
        {
            if (type == PIN_INPUT ||
                type == PIN_ANALOG_INPUT)
            {
                x = pin_ll_get(pin);
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
                        pin_to_iocom_func(pin);
                    }

#if PINS_SIMULATED_INTERRUPTS
                    if (pin->int_conf)
                    {
                        pin_gpio_simulate_interrupt(pin, x);
                    }
#endif
                }
            }
            else
            {
#if PINS_SIMULATED_INTERRUPTS
                if (type == PIN_TIMER)
                {
                    pin_timer_simulate_interrupt(pin);
                }
#endif
                if (pin_to_iocom_func &&
                    pin->signal)
                {
                    pin_to_iocom_func(pin);
                }
            }
        }
    }
}




