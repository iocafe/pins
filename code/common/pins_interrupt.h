/**

  @file    common/pins_interrupt.h
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

struct Pin;

/* Interrupt mode flags:
  - PINS_INT_FALLING	Triggers interrupt when the pin goes from HIGH to LOW
  - PINS_INT_RISING	Triggers interrupt when the pin goes from LOW to HIGH
  - PINS_INT_CHANGE	Triggers interrupt whenever the pin changes value, from HIGH to LOW or LOW to HIGH
 */
#define PINS_INT_FALLING 1
#define PINS_INT_RISING	2
#define PINS_INT_CHANGE	(PINS_INT_FALLING|PINS_INT_RISING)


/* Default interrupt handler defines to be used if OS specific ones are
   not defined (for testing in Windows/Linux).
 */
#ifndef BEGIN_PIN_INTERRUPT_HANDLER
  typedef void pin_interrupt_handler(void);
  #define BEGIN_PIN_INTERRUPT_HANDLER(func_name) void func_name() {
  #define END_PIN_INTERRUPT_HANDLER(func_name) }
  #ifndef PINS_SIMULATED_INTERRUPTS
  #define PINS_SIMULATED_INTERRUPTS 1
  #endif
#endif

/* Structure for simulated configuration.
 */
typedef struct PinInterruptConf
{
    /** Pointer to interrupt handler function.
     */
    pin_interrupt_handler *int_handler_func;

    /** Timer if we are simulating timer interrupts.
     */
    os_timer hit_timer;

    /** Interrupt mode flags.
     */
    os_short flags;
}
PinInterruptConf;

/* If we need to store interrupt and handler configuration by pin.
 */
#if PINS_SIMULATED_INTERRUPTS
#define PINS_INTCONF_STRUCT(name) static PinInterruptConf name;
#define PINS_INTCONF_PTR(name) ,&name
#define PINS_INTCONF_NULL ,OS_NULL
#else
#define PINS_INTCONF_STRUCT(name)
#define PINS_INTCONF_PTR(name)
#define PINS_INTCONF_NULL
#endif


/* Parameter structure for pin_attach_interrupt() function.
 */
typedef struct pinInterruptParams
{
    /** Pointer to interrupt handler function.
     */
    pin_interrupt_handler *int_handler_func;

    /** Interrupt mode flags.
     */
    os_short flags;
}
pinInterruptParams;


/* Attach and interrupt to a GPIO pin.
 */
void pin_attach_interrupt(
    const struct Pin *pin,
    pinInterruptParams *prm);

/* Detach interrupt from GPIO pin.
 */
void pin_detach_interrupt(
    const struct Pin *pin);

#if PINS_SIMULATED_INTERRUPTS

/* Trigger a simulalated interrupt if flags match to x change.
 */
void pin_simulate_interrupt(
    const struct Pin *pin,
    os_int x);

#endif
