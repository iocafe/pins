/**

  @file    common/pins_basics.h
  @brief   Pins library basic functionality.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    8.1.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/

/** Enumeration of pin types.
 */
typedef enum
{
    PIN_INPUT,
    PIN_OUTPUT,
    PIN_ANALOG_INPUT,
    PIN_ANALOG_OUTPUT,
    PIN_PWM,
    PIN_TIMER
}
pinType;

/** Enumeration of possible parameters for the pin.
 */
typedef enum
{
    PIN_RV,        /* Reserved for value */
    PIN_PULL_UP,
    PIN_TOUCH,
    PIN_FREQENCY,
    PIN_RESOLUTION,
    PIN_INIT,
    PIN_INTERRUPT, /* Can specify interrup channel, etc */
    PIN_SPEED,     /* Not used */
    PIN_DELAY,     /* Not used */
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
    const PinGroupHdr **group;
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
    os_short *prm;

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
    /** If we need to simulate HW interrupts, we store the handler function
        pointer here.
     */
    pin_interrupt_handler *int_handler_func;

    /* Flags to specify when interrupt is triggered.
     */


#endif
}
Pin;


/* Setup IO hardware for a device.
 */
void pins_ll_setup(
    const IoPinsHdr *pins_hdr,
    os_int flags);

/* Set IO pin state.
 */
void pin_ll_set(
    const Pin *pin,
    os_int state);

/* Get pin state.
 */
os_int pin_ll_get(
    const Pin *pin);

