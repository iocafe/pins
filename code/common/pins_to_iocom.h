/**

  @file    common/pins_to_iocom.c
  @brief   Functions for connecting IO pins and IOCOM signals.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    3.11.2019

  Copyright 2012 - 2019 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#ifndef PINS_TO_IOCOM_INCLUDED
#define PINS_TO_IOCOM_INCLUDED


/* Flags. Reserved for future expansion.
   For now: always set PINS_DEFAULT (0).
 */
#define PINS_DEFAULT 0

struct iocRoot;
struct iocHandle *handle;

/* Connect pins to IOCOM library
 */
void pins_connect_iocom_library(
    const IoPinsHdr *pins_hdr);

/* Forward signal change to IO.
 */
void forward_signal_change_to_io_pins(
    struct iocHandle *handle,
    int start_addr,
    int end_addr,
    os_ushort flags);

#endif
