/**

  @file    common/pins_basics.h
  @brief   Pins library basic functionality.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    16.9.2019

  Copyright 2012 - 2019 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#ifndef PINS_BASICS_INCLUDED
#define PINS_BASICS_INCLUDED
#include "eosal.h"

/* Enumeration of pin types.
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

/* Enumeration of possible parameters for the pin.
 */
typedef enum
{
    PIN_PULL_UP,
    PIN_TOUCH,
    PIN_FREQENCY,
    PIN_RESOLUTION,
    PIN_INIT,
    PIN_SPEED, /* not used */
    PIN_DELAY, /* not used */
    PIN_MIN,   /* Minimum value for signal */
    PIN_MAX    /* Maximum value for signal, 0 if not set */
}
pinPrm;

struct Pin;

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


/* Structure to set up static information about one IO pin or other IO item.
 */
typedef struct Pin
{
    /* Pointer to pin name string. This may be OS_NULL in future if someone strips out strings
       to save memory.
     */
    os_char *name;

    /* Pint type, like PIN_INPUT, PIN_OUTPUT... See pinType enumeration.
     */
    pinType type;

    /* Hardware bank number for the pin, if applies.
     */
    os_short bank;

    /* Hardware address for the pin.
     */
    os_short addr;

    /* Pointer to parameter array, OS_NULL if none.
     */
    os_short *prm;

    /* Number of items in parameter array. Number of set parameters is this divided by
       two, since each 16 bit number is parameter number amd parameter value.
     */
    os_char prm_n;

    /* Next pin in linked list of pins belonging to same group as this one. OS_NULL
       to indicate that this pin is end of list of not in group.
     */
    const struct Pin *congroup_next;

    /* Next pin in linked list of pins belonging to the same io configuration. OS_NULL
       to indicate that this pin is end of list of not in group.
     */
    // NO LONGER NEEDED const struct Pin *board_next;
}
Pin;


/* Setup hardware IO for a device.
 */
void pins_setup(
    const IoPinsHdr *pins_hdr,
    os_int flags);

/* Set IO pin state.
 */
void pin_set(
    const Pin *pin,
    os_int state);

/* Get pin state.
 */
os_int pin_get(
    const Pin *pin);

#endif
