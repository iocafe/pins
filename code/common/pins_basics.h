/**

  @file    common/pins_basics.h
  @brief   Pins library basic functionality.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    21.4.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#pragma once
#ifndef PINS_BASICS_H_
#define PINS_BASICS_H_
#include "pins.h"

/** Enumeration of pin types.
 */
typedef enum
{
    PIN_INPUT,
    PIN_OUTPUT,
    PIN_ANALOG_INPUT,
    PIN_ANALOG_OUTPUT,
    PIN_PWM,
    PIN_SPI,
    PIN_TIMER,
    PIN_UART,
    PIN_CAMERA
}
pinType;

/* Bit fields for PIN_INTERRUPT_ENABLED parameter.
 */
#define PIN_GLOBAL_INTERRUPTS_ENABLED 1
#define PIN_INTERRUPTS_ENABLED_FOR_PIN 2
#define PIN_GPIO_PIN_INTERRUPTS_ENABLED 4

/** Enumeration of possible parameters for the pin.
 */
typedef enum
{
    PIN_RV,        /* Reserved for value */
    PIN_PULL_UP,
    PIN_PULL_DOWN,
    PIN_TOUCH,
    PIN_FREQENCY,
    PIN_FREQENCY_KHZ,
    PIN_RESOLUTION,
    PIN_INIT,
    PIN_HPOINT,
    PIN_INTERRUPT_ENABLED,
    PIN_TIMER_SELECT,
    PIN_TIMER_GROUP_SELECT,
    PIN_MISO,
    PIN_MOSI,
    PIN_SCLK,
    PIN_CS,
    PIN_DC,
    PIN_RX,
    PIN_TX,
    PIN_TRANSMITTER_CTRL,
    PIN_SPEED,
    PIN_A,
    PIN_B,
    PIN_C,
    PIN_D,
    PIN_E,
    PIN_A_BANK,
    PIN_B_BANK,
    PIN_C_BANK,
    PIN_D_BANK,
    PIN_E_BANK,
    PIN_MIN,       /* Minimum value for signal */
    PIN_MAX        /* Maximum value for signal, 0 if not set */
}
pinPrm;

/** Number of elements in beginning prm array reserved for value
 */
#define PINS_N_RESERVED 2

struct Pin;
struct PinInterruptConf;

typedef struct
{
    os_short n_pins;
    const struct Pin *pin;
}
PinGroupHdr;

typedef struct
{
    const PinGroupHdr * const *group;
    os_short n_groups;
}
IoPinsHdr;

struct iocSignal;

/** Structure to set up static information about one IO pin or other IO item.
 */
typedef struct Pin
{
    /** Pint type, like PIN_INPUT, PIN_OUTPUT... See pinType enumeration.
     */
    os_char type;

    /** Hardware bank number for the pin, if applies.
     */
    os_short bank;

    /** Hardware address for the pin.
     */
    os_short addr;

    /** Pointer to parameter array, two first os_shorts are reserved for storing value
        as os_int.
     */
    os_ushort *prm;

    /** Number of items in parameter array. Number of set parameters is this divided by
        two, since each 16 bit number is parameter number amd parameter value.
     */
    os_char prm_n;

    /** Next pin in linked list of pins belonging to same group as this one. OS_NULL
        to indicate that this pin is end of list of not in group.
     */
    const struct Pin *next;

    /** Pointer to IO signal, if this pin is mapped to one.
     */
    const struct iocSignal *signal;

#if PINS_SIMULATED_INTERRUPTS
    /* Pointer to interrupt configuration when working on simulated environment
     */
    struct PinInterruptConf *int_conf;

#endif
}
Pin;

/* Initialize IO hardware library.
 */
void pins_ll_initialize_lib(
    void);

/* Clean up resources allocated by IO hardware library.
 */
void pins_ll_shutdown_lib(
    void);

/* Setup IO hardware pin.
 */
void pin_ll_setup(
    const Pin *pin,
    os_int flags);

/* Release any resources allocated for IO hardware "pin".
 */
#if OSAL_PROCESS_CLEANUP_SUPPORT
void pin_ll_shutdown(
    const Pin *pin);
#endif

/* Set IO pin state.
 */
void pin_ll_set(
    const Pin *pin,
    os_int x);

/* Get pin state.
 */
os_int pin_ll_get(
    const Pin *pin);

#endif
