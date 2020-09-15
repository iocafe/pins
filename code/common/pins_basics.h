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

struct Pin;
struct PinInterruptConf;
struct PinsBusDevice;

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
    PIN_I2C,
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
    PIN_RV,        /* Reserved for value or state bits */
    PIN_PULL_UP,
    PIN_PULL_DOWN,
    PIN_TOUCH,
    PIN_FREQENCY,
    PIN_FREQENCY_KHZ,
    PIN_FREQENCY_MHZ,
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
    PIN_SDA,
    PIN_SCL,
    PIN_DC,
    PIN_RX,
    PIN_TX,
    PIN_TRANSMITTER_CTRL,
    PIN_SPEED,
    PIN_SPEED_KBPS,
    PIN_FLAGS,
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
    PIN_MAX,       /* Maximum value for signal, 0 if not set */
    PIN_SMIN,      /* Minimum integer value for scaled signal */
    PIN_SMAX,      /* Maximum integer value for scaled signal, 0 if not set */
    PIN_DIGS       /* If pin value is scaled to float, number of decimal digits. Value is divided by 10^n */
}
pinPrm;

/** Number of PinPrm elements (4 bytes) in beginning prm array reserved for PinRV structure (8 bytes)
 */
#define PINS_N_RESERVED 2


/** Pin flags (flags member of Pin structure). PIN_SCALING_SET flag indicates that scaling
    for the PIN value is defined by "smin", "smax" or "digs" attributes.
 */
#define PIN_SCALING_SET 1


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


typedef struct PinPrmValue {
    os_short ix;
    os_short value;
}
PinPrmValue;


/* Since Pin structure is "const" and can be only in flash memory, the PinRV structure is
   used to store dynamic data for IO pin. The PinRV is always 8 bytes and needs to be
   aligned to 4 byte boundary.
 */
typedef struct PinRV {
    os_int value;
    os_char state_bits;
    os_char reserved1;
    os_char reserved2;
    os_char reserved3;
}
PinRV;

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
    os_char bank;

    /** Hardware address for the pin.
     */
    os_short addr;

    /** Pointer to parameter array, two first os_shorts are reserved for storing value
        as os_int.
     */
    PinPrmValue *prm;

    /** Number of items in parameter array. Number of set parameters is this divided by
        two, since each 16 bit number is parameter number amd parameter value.
     */
    os_char prm_n;

    /** Number of items in parameter array. Number of set parameters is this divided by
        two, since each 16 bit number is parameter number amd parameter value.
     */
    os_char flags;

    /** Next pin in linked list of pins belonging to same group as this one. OS_NULL
        to indicate that this pin is end of list of not in group.
     */
    const struct Pin *next;

    /** Pointer to IO signal, if this pin is mapped to one.
     */
    const struct iocSignal *signal;

#if PINS_SPI || PINS_I2C
    /** SPI or IC2 bus device structure.
     */
    struct PinsBusDevice *bus_device;
#endif

#if PINS_SIMULATED_INTERRUPTS
    /** Pointer to interrupt configuration when working on simulated environment
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
    const Pin *pin,
    os_char *state_bits);

/* SPI and I2C initialization.
 */
#if PINS_SPI || PINS_I2C
void pins_initialize_bus_devices(void);
#endif

#endif
