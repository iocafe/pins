/**

  @file    extensions/spi/common/pins_devicebus.h
  @brief   SPI.
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

THINK ABOUT
  - DATA PINS in JSON are not associated directly with any physical pin, but present values
    to read or write as directly connected signals. Data signals can be in any signal group.
  - Data signals can be associated to SPI device.
  - Signals like MISO, MOSI, CLOCK AND CS are real signals with address, but controlled
    by SPI code. Set up same way as camera signals?. Does't work for CS. Or should we have
    "reserved" pins group.
  - How to specify SPI devices in JSON.


  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "pinsx.h"
#if PINS_SPI || PINS_I2C

void pins_init_bus(
    PinsBus *bus)
{

}

#endif


