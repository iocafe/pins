/**

  @file    duino/pins_duino_gpio.c
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

/* Forward referred static functions.
 */
static void pin_gpio_global_interrupt_control(
    os_boolean enable,
    void *context);

static void pin_gpio_set_interrupt_enable_flag(
    const struct Pin *pin,
    os_boolean enable,
    os_int flag);

static void pin_gpio_control_interrupt(
    const struct Pin *pin);


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
    in mode = INPUT;
    if (pin_get_prm(pin, PIN_PULL_UP)) mode = INPUT_PULLUP;
    if (pin_get_prm(pin, PIN_PULL_DOWN)) mode = INPUT_PULLDOWN;
    pinMode(pin->addr, mode);
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
    pinMode(pin->addr, OUTPUT);
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
    int mode;

    switch (prm->flags & PINS_INT_CHANGE)
    {
        case PINS_INT_FALLING: mode = FALLING; break;
        case PINS_INT_RISING:  mode = RISING; break;
        default:
        case PINS_INT_CHANGE: mode = CHANGE; break;
    }

    attachInterrupt(pin->addr, prm->int_handler_func, mode);
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
    detachInterrupt(pin->addr);
}


/**
****************************************************************************************************

  @brief Global enable/disable interrupts callback for flash writes.
  @anchor pin_gpio_global_interrupt_control

  The pin_gpio_global_interrupt_control() function is callback function from
  global interrupt control. The purpose of global control is to disable interrupts
  when writing to flash.

  @param   enable OS_TRUE to mark that interrupts are enabled globally, or OS_FALSE
           to mark that interrupts are disabled.
  @param   context Pointer to the pin structure, callback context.
  @return  None.

****************************************************************************************************
*/
static void pin_gpio_global_interrupt_control(
    os_boolean enable,
    void *context)
{
    const struct Pin *pin;
    pin = (const struct Pin*)context;

    pin_gpio_set_interrupt_enable_flag(pin, enable, PIN_GLOBAL_INTERRUPTS_ENABLED);
    pin_gpio_control_interrupt(pin);
}


/**
****************************************************************************************************

  @brief Helper function to modify PIN_INTERRUPT_ENABLED bits.
  @anchor pin_gpio_global_interrupt_control

  The pin_gpio_global_interrupt_control() function is helper function to
  set or clear a bit in PIN_INTERRUPT_ENABLED parameter.

  Bit PIN_GLOBAL_INTERRUPTS_ENABLED indicates that interrupts are globally enabled
  and we are not now writing to flash.
  Bit PIN_INTERRUPTS_ENABLED_FOR_PIN indicates if interrupts are enabled specifically
  for this pin.

  @param   pin Pointer to the pin structure.
  @param   enable OS_TRUE to set the bit flag, OS_FALSE to clear it.
  @param   flag PIN_GLOBAL_INTERRUPTS_ENABLED or PIN_INTERRUPTS_ENABLED_FOR_PIN
  @return  None.

****************************************************************************************************
*/
static void pin_gpio_set_interrupt_enable_flag(
    const struct Pin *pin,
    os_boolean enable,
    os_int flag)
{
    os_int x;

    x = pin_get_prm(pin, PIN_INTERRUPT_ENABLED);
    if (enable) {
        x |= flag;
    }
    else {
        x &= ~flag;
    }
    pin_set_prm(pin, PIN_INTERRUPT_ENABLED, x);
}


/**
****************************************************************************************************

  @brief Enable or disable interrups for the pin.
  @anchor pin_gpio_control_interrupt

  The pin_gpio_control_interrupt() function enables or disables interrupts
  according PIN_INTERRUPT_ENABLED parameter.

  @param   pin Pointer to the pin structure.
  @return  None.

****************************************************************************************************
*/
static void pin_gpio_control_interrupt(
    const struct Pin *pin)
{
    os_int x;
    x = pin_get_prm(pin, PIN_INTERRUPT_ENABLED);
    if ((x & (PIN_GLOBAL_INTERRUPTS_ENABLED|PIN_INTERRUPTS_ENABLED_FOR_PIN))
        == (PIN_GLOBAL_INTERRUPTS_ENABLED|PIN_INTERRUPTS_ENABLED_FOR_PIN))
    {
        if ((x & PIN_GPIO_PIN_INTERRUPTS_ENABLED) == 0)
        {
            gpio_intr_enable((gpio_num_t)(pin->addr));
            x |= PIN_GPIO_PIN_INTERRUPTS_ENABLED;
            pin_set_prm(pin, PIN_INTERRUPT_ENABLED, x);
        }
    }
    else
    {
        if (x & PIN_GPIO_PIN_INTERRUPTS_ENABLED)
        {
            gpio_intr_disable((gpio_num_t)(pin->addr));
            x &= ~PIN_GPIO_PIN_INTERRUPTS_ENABLED;
            pin_set_prm(pin, PIN_INTERRUPT_ENABLED, x);
        }
    }
}


/* int IRAM_ATTR local_adc1_read_test(int channel) {
    uint16_t adc_value;
    SENS.sar_meas_start1.sar1_en_pad = (1 << channel); // only one channel is selected
    while (SENS.sar_slave_addr1.meas_status != 0);
    SENS.sar_meas_start1.meas1_start_sar = 0;
    SENS.sar_meas_start1.meas1_start_sar = 1;
    while (SENS.sar_meas_start1.meas1_done_sar == 0);
    adc_value = SENS.sar_meas_start1.meas1_data_sar;
    return adc_value;
} */
