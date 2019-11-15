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
#include "pinsx.h"
#include "iocom.h"

static void pin_to_iocom(
    const Pin *pin);


/**
****************************************************************************************************

  @brief Connect PINS library to IOCOM library

  The pins_connect_iocom_library function can be called by ioboard_fc_callback()
  to  function...

  @param   hdr  Static PINS configuration header structure.
  @return  None.

****************************************************************************************************
*/
void pins_connect_iocom_library(
    const IoPinsHdr *hdr)
{
    /* Set function pointer to forward changes.
     */
    pin_to_iocom_func = pin_to_iocom;

    pins_read_all(hdr, PINS_RESET_IOCOM);
}


/**
****************************************************************************************************

  @brief Write pin value as IOCOM signal.

  The pin_to_iocom function stores pin value as IOCOM signal...

  @param   pin Pin configuration structure.
  @return  None.

****************************************************************************************************
*/
static void pin_to_iocom(
    const Pin *pin)
{
    const iocSignal *s;
    os_int x;

    s = pin->signal;

    /* We cannot write to communication target memory nor change signals for it.
     */
    if (s->handle->flags & IOC_TARGET) return;

    /* Set the signal.
     */
    // s->state_bits = OSAL_STATE_CONNECTED;
    x = *(os_int*)pin->prm;
    ioc_sets_int(s, x, OSAL_STATE_CONNECTED);
}


/**
****************************************************************************************************

  @brief Forward signal change to IO.

  The forward_signal_change_to_io_pins function can be called by ioboard_fc_callback()
  to  function...

  @param   handle Memory block handle.
  @param   flags IOC_MBLK_CALLBACK_WRITE indicates change by local write,
           IOC_MBLK_CALLBACK_RECEIVE change by data received. Typically flags of communication
           callback function are passed here.
  @return  None.

****************************************************************************************************
*/
void forward_signal_change_to_io_pins(
    struct iocHandle *handle,
    os_int start_addr,
    os_int end_addr,
    os_ushort flags)
{
    const iocDeviceHdr *device_hdr;
    const iocMblkSignalHdr *mblk_signal_hdr;
    iocMblkSignalHdr **mblk_signal_hdrs;
    const Pin *pin;
    const iocSignal *xsignals, *signal;
    os_int n_signals, j, x;
    os_short n_mblk_hdrs, i;
    os_char state_bits;

    /* If this memory block is not written by communication, no need to do anything.
     */
    if ((handle->flags & IOC_TARGET) == 0 ||
         (flags & IOC_MBLK_CALLBACK_RECEIVE) == 0) return;

    device_hdr = handle->root->device_signal_hdr;

    mblk_signal_hdrs = device_hdr->mblk_hdr;
    n_mblk_hdrs = device_hdr->n_mblk_hdrs;

    /* Loop trough memory block.
     */
    for (i = 0; i < n_mblk_hdrs; i++)
    {
        /* Get signal header, this must be the same memory block.
         */
        mblk_signal_hdr = mblk_signal_hdrs[i];
        if (handle->mblk != mblk_signal_hdr->handle->mblk) continue;

        xsignals = mblk_signal_hdr->first_signal;
        n_signals = mblk_signal_hdr->n_signals;

        /* Loop trough signals in the memory block.
         */
        for (j = 0; j < n_signals; j++)
        {
            signal = xsignals + j;

            /* If not bound to a pin.
             */
            pin = OS_NULL;
            if (signal->flags & IOC_PIN_PTR) pin = (const Pin *)signal->ptr;
            if (pin == OS_NULL) continue;

            if (ioc_is_my_address(signal, start_addr, end_addr))
            {
                x = ioc_gets_int(signal, &state_bits);
                if (state_bits & OSAL_STATE_CONNECTED)
                {
                    pin_ll_set(pin, x);
                    *(os_int*)pin->prm = x;
                }
            }
        }

        /* We found mathing memory block, no need to check further.
         */
        break;
    }
}
