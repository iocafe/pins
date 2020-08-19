/**

  @file    extensions/spi/common/pins_devicebus.c
  @brief   SPI and I2C.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    19.8.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "pinsx.h"
#if PINS_SPI || PINS_I2C


/**
****************************************************************************************************

  @brief Clear chaning state variables in bus structure.
  @anchor pins_init_bus

  The pins_init_bus() function is called at bootup to ensure that there is no old state
  data in memory after a soft reboot. (Many microcontroller do not clear memory at soft reboot).

  @param   buf Pointer to bus structure.
  @return  None.

****************************************************************************************************
*/
void pins_init_bus(
    PinsBus *bus)
{
    os_memclear(&bus->spec, sizeof(PinsBusVariables));
}

#endif


