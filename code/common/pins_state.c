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

        while (pcount--)
        {
#if PINS_SPI || PINS_I2C
            if (pin->bus_device == OS_NULL) {
                pin_ll_setup(pin, flags);
            }
#else
            pin_ll_setup(pin, flags);
#endif

            pin++;
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
  @anchor pin_set_ext

  The pin_set_ext() function writes pin value to IO hardware, stores it for the Pin structure and,
  if appropriate, writes pin value as IOCOM signal.

  @param   pin Pointer to pin configuration structure.
  @param   x Value to set.
  @param   flags PIN_FORWARD_TO_IOCOM to forward change to IOCOM.
  @return  None.

****************************************************************************************************
*/
void pin_set_ext(
    const Pin *pin,
    os_int x,
    os_short flags)
{
#if PINS_SPI || PINS_I2C
    if (pin->bus_device) {
        pin->bus_device->set_func(pin->bus_device, pin->addr, x);

osal_trace_int("~HERE Setting BUS DEVICE pin addr ", pin->addr);
osal_trace_int("HERE to value ", x);

    }
    else {
        pin_ll_set(pin, x);
    }
#else
    pin_ll_set(pin, x);
#endif

    if (flags & PIN_FORWARD_TO_IOCOM)
    {
        if (x != ((PinRV*)pin->prm)->value || ((PinRV*)pin->prm)->state_bits != OSAL_STATE_CONNECTED)
        {
            ((PinRV*)pin->prm)->value = x;
            ((PinRV*)pin->prm)->state_bits = OSAL_STATE_CONNECTED;

            if (pin_to_iocom_func &&
                pin->signal)
            {
                pin_to_iocom_func(pin);
            }
        }
    }
}


/**
****************************************************************************************************

  @brief Set IO pin state.
  @anchor pin_set_scaled

  The pin_set_scaled() function writes pin value to IO hardware, stores it for the Pin structure and,
  if appropriate, writes pin value as IOCOM signal.

  Value is scaled, if scaling is set for the pin.

  @param   pin Pointer to pin configuration structure.
  @param   x Value to set.
  @param   flags PIN_FORWARD_TO_IOCOM to forward change to IOCOM.
  @return  None.

****************************************************************************************************
*/
void pin_set_scaled(
    const Pin *pin,
    os_double x,
    os_boolean flags)
{
    os_double gain;
    os_int minx, maxx, miny, maxy, digs, dx, dy;

    if (pin->flags & PIN_SCALING_SET)
    {
        minx = pin_get_prm(pin, PIN_MIN);
        maxx = pin_get_prm(pin, PIN_MAX);
        miny = pin_get_prm(pin, PIN_SMIN);
        maxy = pin_get_prm(pin, PIN_SMAX);
        digs = pin_get_prm(pin, PIN_DIGS);

        while (digs > 0) {
            x *= 10.0;
            digs--;
        }

        while (digs < 0) {
            x *= 0.1;
            digs++;
        }

        dx = maxx - minx;
        dy = maxy - miny;
        if (dx == 0 || dy == 0) {
            osal_debug_error("Pin value scaling error (set)");
            goto justset;
        }
        gain = (os_double)dx / (os_double)dy;

        x = gain * (x - miny) + minx;
    }
justset:
    pin_set_ext(pin, os_round_int(x), flags);
}


/**
****************************************************************************************************

  @brief Get pin value and state bits.
  @anchor pin_get_ext

  The pin_get() function reads pin value to IO hardware, stores it for the Pin structure and,
  if appropriate, writes the pin value as IOCOM signal.

  @param   pin Pointer to pin configuration structure.
  @param   state_bits Pointer to byte where to store state bits, Set OS_NULL if not needed.
  @return  Pin value from IO hardware. -1 if value is not available (not read, errornous, etc.).

****************************************************************************************************
*/
os_int pin_get_ext(
    const Pin *pin,
    os_char *state_bits)
{
    os_int x;
    os_char tmp_state_bits;

    if (state_bits == OS_NULL) {
        state_bits = &tmp_state_bits;
    }

#if PINS_SPI || PINS_I2C
    if (pin->bus_device) {
        x = pin->bus_device->get_func(pin->bus_device, pin->addr, state_bits);
    }
    else {
        x = pin_ll_get(pin, state_bits);
    }
#else
    x = pin_ll_get(pin, state_bits);
#endif

    if (x != ((PinRV*)pin->prm)->value ||
        *state_bits != ((PinRV*)pin->prm)->state_bits)
    {
        ((PinRV*)pin->prm)->value = x;
        ((PinRV*)pin->prm)->state_bits = *state_bits;

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

  @brief Get scaled pin value.
  @anchor pin_get_scaled

  The pin_get_scaled() function reads pin value to IO hardware, stores it for the Pin structure
  and, if appropriate, writes the pin value as IOCOM signal.

  If the pin has scaling set by "min", "max", "dmin", "dmax", "digs", value is scaled,

  @param   pin Pointer to pin configuration structure.
  @return  Pin value from IO hardware. -1 if value is not available (not read, errornous, etc.).

****************************************************************************************************
*/
os_double pin_get_scaled(
    const Pin *pin,
    os_char *state_bits)
{
    os_int ivalue;

    ivalue = pin_get_ext(pin, state_bits);
    if ((pin->flags & PIN_SCALING_SET) == 0) {
        return ivalue;
    }
    return pin_value_scaled(pin, OS_NULL);
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
  @param   state_bits Pointer to byte where to store state bits, Set OS_NULL if not needed.
  @return  Memorized pin value.

****************************************************************************************************
*/
os_int pin_value(
    const Pin *pin,
    os_char *state_bits)
{
    if (state_bits) {
        *state_bits = ((PinRV*)pin->prm)->state_bits;
    }

    return ((PinRV*)pin->prm)->value;
}


/**
****************************************************************************************************

  @brief Get scaled pin value.
  @anchor pin_value_scaled

  The pin_value_scaled() function is like pin_value, but does scale the value
  "min", "max", "dmin", "dmax" and "digs",

  If the pin has scaling set by "min" - "dmin", "dmax", "digs", value is scaled,

  @param   pin Pointer to pin configuration structure.
  @return  Pin value from IO hardware. -1 if value is not available (not read, errornous, etc.).

****************************************************************************************************
*/
os_double pin_value_scaled(
    const Pin *pin,
    os_char *state_bits)
{
    os_double gain, dvalue;
    os_int ivalue, minx, maxx, miny, maxy, digs, dx, dy;

    ivalue = pin_value(pin, state_bits);
    if ((pin->flags & PIN_SCALING_SET) == 0) {
        return ivalue;
    }

    minx = pin_get_prm(pin, PIN_MIN);
    maxx = pin_get_prm(pin, PIN_MAX);
    miny = pin_get_prm(pin, PIN_SMIN);
    maxy = pin_get_prm(pin, PIN_SMAX);
    digs = pin_get_prm(pin, PIN_DIGS);

    dx = maxx - minx;
    dy = maxy - miny;
    if (dx == 0 || dy == 0) {
        osal_debug_error("Pin value scaling error");
        return ivalue;
    }
    gain = (os_double)dy / (os_double)dx;

    dvalue = gain * (ivalue - minx) + miny;

    while (digs > 0) {
        dvalue *= 0.1;
        digs--;
    }

    while (digs < 0) {
        dvalue *= 10.0;
        digs++;
    }

    return dvalue;
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
    os_char type, state_bits;

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
#if PINS_SPI || PINS_I2C
                if (pin->bus_device) {
                    x = pin->bus_device->get_func(pin->bus_device, pin->addr, &state_bits);
                }
                else {
                    x = pin_ll_get(pin, &state_bits);
                }
#else
                x = pin_ll_get(pin, &state_bits);
#endif

                if (x != ((PinRV*)pin->prm)->value ||
                    state_bits != ((PinRV*)pin->prm)->state_bits ||
                    (flags & PINS_RESET_IOCOM))
                {
                    ((PinRV*)pin->prm)->value = x;
                    ((PinRV*)pin->prm)->state_bits = state_bits;

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


/**
****************************************************************************************************

  @brief Read group of pins
  @anchor pins_read_group

  The pins_read_group() can be called to pass read a set of pins, for example to pass analog
  inputs to IOCOM.

  @param   pin Pointer to first pin of the group.
  @return  None.

****************************************************************************************************
*/
void pins_read_group(
    const Pin *pin)
{
    os_char state_bits;

    while (pin) {
        pin_get_ext(pin, &state_bits);
        pin = pin->next;
    }
}
