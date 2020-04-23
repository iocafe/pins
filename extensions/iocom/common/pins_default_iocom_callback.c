/**

  @file    extensions/iocom/common/pins_to_iocom.c
  @brief   Functions for connecting IO pins and IOCOM signals.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    21.4.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "pinsx.h"
#include "iocom.h"

/**
****************************************************************************************************

  @brief Forward data data received from communication to IO pins.

  The pins_default_iocom_callback function is default implementation of callback from IOCOM.

  @param   handle Memory block handle.
  @param   start_addr First changed memory block address.
  @param   end_addr Last changed memory block address.
  @param   flags IOC_MBLK_CALLBACK_WRITE indicates change by local write,
           IOC_MBLK_CALLBACK_RECEIVE change by data received.
  @param   context Callback context, not used by "dref" example.
  @return  None.

****************************************************************************************************
*/
void pins_default_iocom_callback(
    struct iocHandle *handle,
    os_int start_addr,
    os_int end_addr,
    os_ushort flags,
    void *context)
{
    /* Call pins library extension to forward communication signal changes to IO pins.
     */
    if (flags & IOC_MBLK_CALLBACK_RECEIVE)
    {
        forward_signal_change_to_io_pins(handle, start_addr, end_addr, flags);
    }
}
