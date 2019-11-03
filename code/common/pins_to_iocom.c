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

/**
****************************************************************************************************

  @brief Forward signal change to IO.

  The forward_signal_change_to_io_pins function can be called by ioboard_fc_callback()
  to  function...

  @param   handle Memory block handle.
  @return  None.

****************************************************************************************************
*/
void forward_signal_change_to_io_pins(
    struct iocHandle *handle,
    int start_addr,
    int end_addr,
    os_ushort flags)
{
//    int i;

    /* if (ioc_is_my_address(&gina.down.seven_segment, start_addr, end_addr))
    {
        ioc_gets_array(&gina.down.seven_segment, buf, N_LEDS);
        if (ioc_is_value_connected(gina.down.seven_segment))
        {
            osal_console_write("7 segment data received\n");
            for (i = 0; i < N_LEDS; i++)
            {
                // digitalWrite(leds[s + i], buf[i] ? HIGH : LOW);
            }
        }
        else
        {
            // WE DO NOT COME HERE. SHOULD WE INVALIDATE WHOLE MAP ON DISCONNECT?
            osal_console_write("7 segment data DISCONNECTED\n");
        }
    } */
}
