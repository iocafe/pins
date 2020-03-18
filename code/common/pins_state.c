/**

  @file    common/pins_state.c
  @brief   Set and get pin states.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    8.1.2020

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

/* Forward referred static functions.
 */
#if PINS_SIMULATED_INTERRUPTS
static void pin_simulate_interrupt(
    const struct Pin *pin,
    os_int x);
#endif



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
                        pin_simulate_interrupt(pin, x);
                    }
#endif
                }
            }
            else
            {
                if (pin_to_iocom_func &&
                    pin->signal)
                {
                    pin_to_iocom_func(pin);
                }
            }
        }
    }
}


#if PINS_SIMULATED_INTERRUPTS
/**
****************************************************************************************************

  @brief Trigger simulalated interrupt if flags match.
  @anchor pin_simulate_interrupt

  The pin_simulate_interrupt function triggers a simulated interrupt if:
  - x is zero and PINS_INT_FALLING flag is set (included in PINS_INT_CHANGE).
  - x is nonzero and PINS_INT_RISING flag is set (included in PINS_INT_CHANGE).

  @param   pin The GPIO pin structure.
  @param   x New pin state.
  @return  None.

****************************************************************************************************
*/
static void pin_simulate_interrupt(
    const struct Pin *pin,
    os_int x)
{
    os_short flags;

    /* If pin is not configured for interrupts.
     */
    if (pin->int_conf == OS_NULL)
    {
        osal_debug_error("pin_simulate_interrupt: NULL int_conf pointer");
        return;
    }

    /* If interrupt handler not set, just return.
     */
    if (pin->int_conf->int_handler_func == OS_NULL) return;

    /* If new signal value matches rising/falling edge flag, call interrupt handler.
     */
    flags = pin->int_conf->flags;
    if (((flags & PINS_INT_FALLING) && x == 0) ||
        ((flags & PINS_INT_RISING) && x != 0))
    {
        pin->int_conf->int_handler_func();
    }
}
#endif
