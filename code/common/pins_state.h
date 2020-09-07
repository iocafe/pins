/**

  @file    common/pins_state.h
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
#pragma once
#ifndef PINS_STATE_H_
#define PINS_STATE_H_
#include "pins.h"

/* Flags for functions.
 */
#define PINS_DEFAULT 0
#define PINS_RESET_IOCOM 1

/* Function type to forward changes to iocom
 */
typedef void pin_to_iocom_t(
    const Pin *pin);

/* Function pointer to move a pin value to iocom. OS_NULL if not connected to IOCOM.
 */
extern pin_to_iocom_t *pin_to_iocom_func;

/* Setup IO hardware for a device.
 */
void pins_setup(
    const IoPinsHdr *pins_hdr,
    os_int flags);

/* Shut down the hardware IO.
 */
#if OSAL_PROCESS_CLEANUP_SUPPORT
void pins_shutdown(
    const IoPinsHdr *pins_hdr);
#else
    #define pins_shutdown(h)
#endif

/* Set IO pin state.
 */
void pin_set(
    const Pin *pin,
    os_int x);

/* Get pin value.
 */
#define pin_get(pin) pin_get_ext(pin, OS_NULL)

/* Get pin value and state bits.
 */
os_int pin_get_ext(
    const Pin *pin,
    os_char *state_bits);

/* Get scaled pin value
 */
os_double pin_get_scaled(
    const Pin *pin,
    os_char *state_bits);

/* Get pin value without reading hardware.
 */
os_int pin_value(
    const Pin *pin,
    os_char *state_bits);

/* Get scaled pin value without reading hardware.
 */
os_double pin_value_scaled(
    const Pin *pin,
    os_char *state_bits);

/* Read all inputs of the IO device into global Pin structurees
 */
void pins_read_all(
    const IoPinsHdr *hdr,
    os_ushort flags);

/* Read group of pins
 */
void pins_read_group(
    const Pin *pin);


#endif
