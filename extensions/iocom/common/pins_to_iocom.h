/**

  @file    extensions/iocom/common/pins_to_iocom.c
  @brief   Functions for connecting IO pins and IOCOM signals.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    26.4.2021

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#pragma once
#ifndef PINS_TO_IOCOM_H_
#define PINS_TO_IOCOM_H_
#include "pinsx.h"

/* Flags. Reserved for future expansion.
   For now: always set PINS_DEFAULT (0).
 */
#define PINS_DEFAULT 0

struct iocRoot;
struct iocHandle;
struct iocDeviceHdr;

/* Connect pins to IOCOM library
 */
void pins_connect_iocom_library(
    const IoPinsHdr *pins_hdr);

/* Forward signal change to IO. THIS FUNCTION WILL BE OBSOLETED, replaced by faster implementation
 */
void forward_signal_change_to_io_pins(
    struct iocHandle *handle,
    os_int start_addr,
    os_int end_addr,
    const struct iocDeviceHdr *device_hdr,
    os_ushort flags);

/* Forward signal change to IO.
 */
void forward_signal_change_to_io_pin(
    const iocSignal *sig,
    os_short flags);

/* Forward data data received from communication to IO pins.
 */
void pins_default_iocom_callback(
    struct iocHandle *handle,
    os_int start_addr,
    os_int end_addr,
    os_ushort flags,
    void *context);

#endif
