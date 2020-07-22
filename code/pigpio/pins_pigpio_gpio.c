/**

  @file    pigpio/pins_pigpio_gpio.c
  @brief   GPIO pins.
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
#include <pigpio.h>


/**
****************************************************************************************************

  @brief Configure a pin as digital input.
  @anchor pin_gpio_setup_input

  The pin_gpio_setup_input() function...

  @param   pin Pointer to pin structure.
  @return  None.

****************************************************************************************************
*/
void pin_gpio_setup_input(
    const Pin *pin)
{
    unsigned pud;

    if (gpioSetMode((unsigned)pin->addr, PI_INPUT)) {
        osal_debug_error_int("gpioSetMode(x,PI_INPUT) failed, x=", pin->addr);
    }
    else
    {
        pud = PI_PUD_OFF;
        if (pin_get_prm(pin, PIN_PULL_DOWN)) {
            pud = PI_PUD_DOWN;
        }
        else if (pin_get_prm(pin, PIN_PULL_UP)) {
            pud = PI_PUD_UP;
        }
        if (gpioSetPullUpDown((unsigned)pin->addr, pud)) {
            osal_debug_error_int("gpioSetPullUpDown(x,pud) failed, x=", pin->addr);
        }
    }
}


/**
****************************************************************************************************

  @brief Configure a pin as digital output.
  @anchor pin_gpio_setup_output

  The pin_gpio_setup_output() function...

  @param   pin Pointer to pin structure.
  @return  None.

****************************************************************************************************
*/
void pin_gpio_setup_output(
    const Pin *pin)
{
    if (gpioSetMode((unsigned)pin->addr, PI_OUTPUT)) {
        osal_debug_error_int("gpioSetMode(x, PI_OUTPUT) failed, x=", pin->addr);
    }
}


/**
****************************************************************************************************

  @brief Attach an interrupt to a GPIO pin.
  @anchor pin_gpio_attach_interrupt

  The pin_gpio_attach_interrupt() function to set an interrupt handler function on a pin by pin basis.

  There may be HW specific parameters and limitations for reserving interrupt channels, etc.
  The HW specific parameters are in JSON file for the hardware.

  @param   pin The GPIO pin structure.
  @param   prm Parameter structure, contains pointer to interrupt handler function that
           will be called every time the interrupt is triggered.
  @param   flags Flags to specify when interrupt is triggered.
  @return  None.

****************************************************************************************************
*/
void pin_gpio_attach_interrupt(
    const struct Pin *pin,
    pinInterruptParams *prm)
{
    // IN PI WE USE ALERT FUNCTIONS, NOT INTERRUPTS
    // int gpioSetAlertFunc(unsigned user_gpio, gpioAlertFunc_t f)
    // Registers a function to be called (a callback) when the specified GPIO changes state.
    // OR gpioSetAlertFuncEx(unsigned user_gpio, gpioAlertFuncEx_t f, void *userdata)
    // OR nt gpioSetISRFuncEx(unsigned gpio, unsigned edge, int timeout, gpioISRFuncEx_t f, void *userdata)

#if 0
    gpio_int_type_t itype;
    gpio_num_t addr;
    os_boolean enable;
    addr = (gpio_num_t)pin->addr;

    /* gpio_isr_handler_add(addr,  (gpio_isr_t)(prm->int_handler_func), NULL); */
    gpio_isr_handler_add(addr, prm->int_handler_func, NULL);

    switch (prm->flags & PINS_INT_CHANGE)
    {
        case PINS_INT_FALLING: itype = GPIO_INTR_NEGEDGE; break;
        case PINS_INT_RISING:  itype = GPIO_INTR_POSEDGE; break;
        default:
        case PINS_INT_CHANGE: itype = GPIO_INTR_ANYEDGE; break;
    }
    gpio_set_intr_type(addr, itype);

    /* Start listening for global interrupt enable functions.
       It is important to clear PIN_INTERRUPT_ENABLED for soft reboot.
     */
    pin_set_prm(pin, PIN_INTERRUPT_ENABLED, 0);
    enable = osal_add_interrupt_to_list(pin_gpio_global_interrupt_control, (void*)pin);
    pin_gpio_set_interrupt_enable_flag(pin, enable, PIN_GLOBAL_INTERRUPTS_ENABLED);
    pin_gpio_set_interrupt_enable_flag(pin, OS_TRUE, PIN_INTERRUPTS_ENABLED_FOR_PIN);

    pin_gpio_control_interrupt(pin);
#endif
}


/**
****************************************************************************************************

  @brief Detach interrupt from GPIO pin.
  @anchor pin_gpio_detach_interrupt

  You can call pin_gpio_detach_interrupt() function when you no longer want to receive interrupts
  related to the pin.

  @param   pin Pointer to the pin structure.
  @return  None.

****************************************************************************************************
*/
void pin_gpio_detach_interrupt(
    const struct Pin *pin)
{
    // pin_gpio_set_interrupt_enable_flag(pin, OS_FALSE, PIN_INTERRUPTS_ENABLED_FOR_PIN);
    // pin_gpio_control_interrupt(pin);

    // gpio_intr_disable(addr);
    // gpio_isr_handler_remove(addr);
}
