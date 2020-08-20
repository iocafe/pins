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


/* Forward referred static functions.
 */
static osalStatus pins_do_bus_transaction(
    PinsBusDevice *device);

static osalStatus pins_bus_run_spi(
    PinsBus *bus);


/**
****************************************************************************************************

  @brief Clear state variables in the SPI/I2C bus structure and initialize the bus.
  @anchor pins_init_bus

  The pins_init_bus() function initializes the SPI/I2C bus and clears old state data.
  (many microcontroller do not clear memory at soft reboot).

  @param   buf Pointer to bus structure.
  @return  None.

****************************************************************************************************
*/
void pins_init_bus(
    PinsBus *bus)
{
    os_memclear(&bus->spec, sizeof(PinsBusVariables));

    // h = pi.spi_open(0, 2e6, 0xE0) # 0xE0 says not to set chip enables

    /* Start from the first device.
     */
    bus->current_device = bus->first_bus_device;
    if (bus->current_device == OS_NULL) {
        osal_debug_error("SPI/I2C bus without devices?");
        return;
    }

    /* Start from first bus in sigle thread mode.
     */
    pins_devicebus.current_bus = pins_devicebus.first_bus;

#if PINS_SPI
    if (bus->bus_type == PINS_SPI_BUS)
    {
        // set speed
        //  current_device : digitalWrite(CS_MCP3208, 0);  // Low : CS active

        /* If we have only one device, we do not need toggle speed and chip selects.
         */
        bus->spec.spi.more_than_1_device = (os_boolean)(bus->current_device->next_device != OS_NULL);
    }
#endif

#if PINS_I2C
    if (bus->bus_type == PINS_I2C_BUS)
    {
    }
#endif
}


/**
****************************************************************************************************

  @brief Run devicebus in single thread system.
  @anchor pins_run_devicebus

  Single threaded mode. Call from main loop to run device bus.

  @param   flags Reserved for future, set zero for now.
  @return  None.

****************************************************************************************************
*/
void pins_run_devicebus(
    os_int flags)
{
    PinsBus *bus;
    osalStatus s;

    bus = pins_devicebus.current_bus;
    s = pins_bus_run_spi(bus);
    if (s == OSAL_COMPLETED) {
        bus = bus->next_bus;
        if (bus == OS_NULL) {
            bus = pins_devicebus.first_bus;
        }
        pins_devicebus.current_bus = bus;
    }
}


#if OSAL_MULTITHREAD_SUPPORT

/**
****************************************************************************************************

  @brief Device bus thread function.
  @anchor ioc_devicebus_thread

  The ioc_devicebus_thread() function is worker thread which runs one SPI or I2C bus.

  @param   prm Pointer to parameters for new thread, pointer to end point object.
  @param   done Event to set when parameters have been copied to entry point
           functions own memory.

  @return  None.

****************************************************************************************************
*/
static void ioc_devicebus_thread(
    void *prm,
    osalEvent done)
{
    PinsBus *bus;
    osalStatus s;

    osal_trace("end point: worker thread created");

    /* Add to devicebus thread count.
     */
    pins_devicebus.thread_count++;

    /* Which device bus to run.
     */
    bus = (PinsBus*)prm;

    /* Let thread which created this one proceed.
     */
    osal_event_set(done);

#if PINS_SPI
    if (bus->bus_type == PINS_SPI_BUS)
    {
        /* Run the the device bus, until program or SPI/I2C communication is
           to be terminated.
         */
        while (osal_go() && !pins_devicebus.terminate)
        {
            s = pins_bus_run_spi(bus);
            if (s == OSAL_COMPLETED) {
                os_timeslice();
            }
        }
    }
#endif

#if PINS_I2C
    if (bus->bus_type == PINS_I2C_BUS)
    {
    }
#endif

    /* This thread will be no longer running, decrement thread count.
     */
    pins_devicebus.thread_count-- ;
}


/**
****************************************************************************************************

   @brief Start multithreaded devicebus.
   @anchor pins_start_multithread_devicebus

   The pins_start_multithread_devicebus() function starts a thread for each SPI or I2C bus.

   @param   flags Reserved for future, set zero for now.
   @return  None.

****************************************************************************************************
*/
void pins_start_multithread_devicebus(
    os_int flags)
{
    PinsBus *bus;

    pins_devicebus.thread_count = 0;
    pins_devicebus.terminate = OS_FALSE;

    for (bus = pins_devicebus.first_bus;
         bus;
         bus = bus->next_bus)
    {
        osal_thread_create(ioc_devicebus_thread, bus, OS_NULL, OSAL_THREAD_DETACHED);
    }
}


/**
****************************************************************************************************

   @brief Stop devicebus threads.
   @anchor pins_stop_multithread_devicebus

   The pins_stop_multithread_devicebus() function requests all devicebus worker threads to
   terminate and waits until they are finished.

   @return  None.

****************************************************************************************************
*/
void pins_stop_multithread_devicebus(
    void)
{
    pins_devicebus.terminate = OS_TRUE;
    while (pins_devicebus.thread_count)
    {
        os_sleep(50);
    }
}

#endif


/**
****************************************************************************************************

   @brief Send data to SPI or I2C bus and receive reply.
   @anchor pins_do_bus_transaction

   The pins_do_bus_transaction() function sends a message to current SPI or I2C device and
   gets a reply. If multiple messages are used with the device, gen_req_func() and
   proc_resp_func() functions process one of these at the time.

   @param   device Pointer to SPI/I2C device structure.
   @return  OSAL_COMPLETED if this was the last IO message to this device. OSAL_SUCCESS otherwise.

****************************************************************************************************
*/
static osalStatus pins_do_bus_transaction(
    PinsBusDevice *device)
{
    osalStatus s;

    device->gen_req_func(device);

    s = device->proc_resp_func(device);
    //  digitalWrite(CS_MCP3208, 0);  // Low : CS Active

    // wiringPiSPIDataRW(SPI_CHANNEL, buff, 3);
    return s;
}


/**
****************************************************************************************************

   @brief Send one SPI bus request and receive reply.
   @anchor pins_do_spi_bus_transaction

   The pins_do_spi_bus_transaction() function sends buffer content to SPI bus gets a reply.
   Here we give turn to every SPI/I2C device and every message for the device. But one call
   to this function transfers only one request/reply pair.

   @param   device Pointer to SPI/I2C device structure.
   @return  OSAL_COMPLETED if this was the last IO message to this of the last IO device device.
            OSAL_SUCCESS otherwise.

****************************************************************************************************
*/
static osalStatus pins_bus_run_spi(
    PinsBus *bus)
{
    PinsBusDevice *current_device;
    osalStatus s, final_s = OSAL_SUCCESS;

    current_device = bus->current_device;

    s = pins_do_bus_transaction(current_device);

    /* If moving to next device
     */
    if (s == OSAL_COMPLETED) {
        /* Disable chip select, if we have more than 1 device.
         */
        if (bus->spec.spi.more_than_1_device) {
            //  current_device : digitalWrite(CS_MCP3208, 1);  // High : CS disable
        }

        /* Move on to the next device.
         */
        current_device = current_device->next_device;
        if (current_device == OS_NULL) {
            current_device = bus->first_bus_device;
            final_s = OSAL_COMPLETED;
        }

        if (bus->spec.spi.more_than_1_device) {
            // set speed
            // current_device : digitalWrite(CS_MCP3208, 0);  // Low : CS active
        }

        bus->current_device = current_device;
    }

    return final_s;
}

#endif
