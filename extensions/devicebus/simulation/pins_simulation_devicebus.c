/**

  @file    extensions/spi/simulation/pins_simulation_devicebus.c
  @brief   SPI and I2C
  @author  Pekka Lehtikoski
  @version 1.0
  @date    16.8.2020

  - Run sequentially. Set speed and chip select pin. Send request. Wait for reply.
    Get reply and process it. The pinsGenerateDeviceRequest() is called to generate the request
    and pinsProcessDeviceResponce() to process it. Main loop calls this for every SPI device in turn.
  - Threaded variation. Much the same as previous one, but every SPI bus has own thread which
    runs the SPI bus sequence, one device at a time: "run set speed and cs, send request, wait,
    get reply and process it."
  - Interrupt based variation. Each SPI bus is run as state machine. State information contains
    which SPI device in this bus has the turn. And are we waiting for speed setting to take
    affect, waiting for reply, etc. Interrupt is timed, we never wait in interrup handler,
    just return is thing waited for is not yet ready.
  - We can also run SPI without waiting from the single threaded main loop.

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
  data in memory after a soft reboot (many microcontroller do not clear memory at soft reboot).

  @param   buf Pointer to bus structure.
  @return  None.

****************************************************************************************************
*/
void pins_init_bus(
    PinsBus *bus)
{
    os_memclear(&bus->spec, sizeof(PinsBusVariables));

    // h = pi.spi_open(0, 2e6, 0xE0) # 0xE0 says not to set chip enables
}


/* Single threaded use. Call from main loop to run device bus
 */
void pins_run_devicebus(
    os_int flags)
{
    int i;
    i= 1;
}


#if OSAL_MULTITHREAD_SUPPORT
/* Run multi threaded device bus. The function starts thread for each SPI bus.
 */
void pins_start_multithread_devicebus(
    os_int flags)
{
    int i;
    i= 1;
}
#endif


/**
****************************************************************************************************

   @brief Send data to SPI bus and receive reply.
   @anchor pins_do_spi_bus_transaction

   The pins_do_spi_bus_transaction() function sends buffer content to SPI bus
   and ...

   @param   photo Pointer to photo to store. The photo is not modified by this function.
   @param   b Pointer to brick buffer into which to store the photo as "brick".
   @param   compression Should photo be compressed by this function and is so how?
   @return  None.

****************************************************************************************************
*/
void pins_do_spi_bus_transaction(
    PinsBus *spi_bus)
{
    //  digitalWrite(CS_MCP3208, 0);  // Low : CS Active

    // wiringPiSPIDataRW(SPI_CHANNEL, buff, 3);
}

#endif
