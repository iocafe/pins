/**

  @file    pigpio/pins_pigpio_timer.c
  @brief   Timer interrups.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    21.7.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "pins.h"
#ifdef PINS_PIGPIO

#include <pigpio.h>

/* Forward referred static functions.
 */
static void pin_timer_set_interrupt_enable_flag(
   const struct Pin *pin,
   os_boolean enable,
   os_int flag);

static void pin_timer_control_interrupt(
    const struct Pin *pin);


/**
****************************************************************************************************

  @brief Attach an interrupt to a timer.
  @anchor pin_timer_attach_interrupt

  The pin_timer_attach_interrupt() function sets an interrupt handler function for
  timer interrupts. The function also configures the timer.

  There may be HW specific parameters and limitations for reserving timers, channels, etc.
  The HW specific parameters are in JSON file for the hardware.

  @param   pin Pointer to the pin structure for the timer.
  @param   prm Parameter structure, contains pointer to interrupt handler function that
           will be called at timer hit.
  @return  None.

****************************************************************************************************
*/
void pin_timer_attach_interrupt(
    const struct Pin *pin,
    pinTimerParams *prm)
{
    // int gpioSetTimerFuncEx(unsigned timer, unsigned millis, gpioTimerFuncEx_t f, void *userdata)


}


/**
****************************************************************************************************

  @brief Detach interrupt from timer.
  @anchor pin_timer_detach_interrupt

  The pin_timer_detach_interrupt() function detaches interrupt, at minimum it disables
  the timer interrupt.

  @param   pin Pointer to the pin structure.
  @return  None.

****************************************************************************************************
*/
void pin_timer_detach_interrupt(
    const struct Pin *pin)
{
    pin_timer_set_interrupt_enable_flag(pin, OS_FALSE, PIN_INTERRUPTS_ENABLED_FOR_PIN);
    pin_timer_control_interrupt(pin);
}


/**
****************************************************************************************************

  @brief Helper function to modify PIN_INTERRUPT_ENABLED bits.
  @anchor pin_timer_set_interrupt_enable_flag

  The pin_timer_set_interrupt_enable_flag() function is helper function to
  set or clear a bit in PIN_INTERRUPT_ENABLED parameter.

  Bit PIN_GLOBAL_INTERRUPTS_ENABLED indicates that interrupts are globally enabled
  and we are not now writing to flash.
  Bit PIN_INTERRUPTS_ENABLED_FOR_PIN indicates if interrupts are enabled specifically
  for this timer.

  @param   pin Pointer to timer's pin structure.
  @param   enable OS_TRUE to set the bit flag, OS_FALSE to clear it.
  @param   flag PIN_GLOBAL_INTERRUPTS_ENABLED or PIN_INTERRUPTS_ENABLED_FOR_PIN
  @return  None.

****************************************************************************************************
*/
static void pin_timer_set_interrupt_enable_flag(
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

  @brief Enable or disable interrups for the timer.
  @anchor pin_timer_control_interrupt

  The pin_timer_control_interrupt() function enables or disables interrupts
  according PIN_INTERRUPT_ENABLED parameter.

  @param   pin Pointer to timer's pin structure.
  @return  None.

****************************************************************************************************
*/
static void pin_timer_control_interrupt(
    const struct Pin *pin)
{
}

#endif
