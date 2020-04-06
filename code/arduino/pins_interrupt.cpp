/**

  @file    arduino/pins_interrupt.c
  @brief   Interrups and handlers.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    16.3.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "pins.h"
#include "Arduino.h"

#ifdef ESP_PLATFORM
#include "driver/gpio.h"
#endif

/**
****************************************************************************************************

  @brief Attach and interrupt to a GPIO pin.
  @anchor pin_attach_interrupt

  The pin_attach_interrupt() function to set an interrupt handler function on a pin by pin basis.

  There may be HW specific parameters and limitations for reserving interrupt channels, etc.
  The HW specific parameters are in JSON file for the hardware.

  @param   pin The GPIO pin structure.
  @param   prm Parameter structure, contains pointer to interrupt handler functio that
           will be called every time the interrupt is triggered.
  @param   flags Flags to specify when interrupt is triggered.
  @return  None.

****************************************************************************************************
*/
void pin_attach_interrupt(
    const struct Pin *pin,
    pinInterruptParams *prm)
{
#ifdef ESP_PLATFORM
    gpio_int_type_t itype;
    gpio_num_t addr;
    addr = (gpio_num_t)pin->addr;

    gpio_isr_handler_add(addr,  (gpio_isr_t)(prm->int_handler_func), NULL);

    switch (prm->flags & PINS_INT_CHANGE)
    {
        case PINS_INT_FALLING: itype = GPIO_INTR_NEGEDGE; break;
        case PINS_INT_RISING:  itype = GPIO_INTR_POSEDGE; break;
        default:
        case PINS_INT_CHANGE: itype = GPIO_INTR_ANYEDGE; break;
    }
    gpio_set_intr_type(addr, itype);

    gpio_intr_enable(addr);

#else
    int mode;

    switch (prm->flags & PINS_INT_CHANGE)
    {
        case PINS_INT_FALLING: mode = FALLING; break;
        case PINS_INT_RISING:  mode = RISING; break;
        default:
        case PINS_INT_CHANGE: mode = CHANGE; break;
    }

    attachInterrupt(pin->addr, prm->int_handler_func, mode);
#endif
}


/**
****************************************************************************************************

  @brief Detach interrupt from GPIO pin.
  @anchor pin_detach_interrupt

  You can call pin_detach_interrupt() function when you no longer want to receive interrupts
  related to the pin.

  @return  None.

****************************************************************************************************
*/
void pin_detach_interrupt(
    const struct Pin *pin)
{
#ifdef ESP_PLATFORM
    gpio_num_t addr;
    addr = (gpio_num_t)pin->addr;

    gpio_intr_disable(addr);
    gpio_isr_handler_remove(addr);

#else
    detachInterrupt(pin->addr);
#endif
}


