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
    affect, waiting for reply, etc. Interrupt is timed, we never wait in interrupt handler,
    just return is thing waited for is not yet ready.
  - We can also run SPI without waiting from the single threaded main loop.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "pinsx.h"
#ifdef PINS_SIMULATE_HW
#if PINS_SPI || PINS_I2C


/* Forward referred static functions.
 */
static osalStatus pins_spi_transfer(
    PinsBusDevice *device);

static osalStatus pins_bus_run_spi(
    PinsBus *bus);

static osalStatus pins_i2c_transfer(
    PinsBusDevice *device);

static osalStatus pins_bus_run_i2c(
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
    PinsBusDevice *device;
#if OSAL_DEBUG
    os_char buf[96], nbuf[OSAL_NBUF_SZ];
#endif

    /* Clear sub type specific data and start from the first device.
     */
    os_memclear(&bus->spec, sizeof(PinsBusVariables));
    device = bus->first_bus_device;
    bus->current_device = device;
    if (device == OS_NULL) {
        osal_debug_error("SPI/I2C bus without devices?");
        return;
    }

    /* Start from first bus in sigle thread mode.
     */
    pins_devicebus.current_bus = pins_devicebus.first_bus;

#if PINS_SPI
    if (bus->bus_type == PINS_SPI_BUS)
    {
        /* Get GPIO pin numbers and optional bus number.
         */
        bus->spec.spi.miso = (os_short)pin_get_prm(device->device_pin, PIN_MISO);
        bus->spec.spi.mosi = (os_short)pin_get_prm(device->device_pin, PIN_MOSI);
        bus->spec.spi.sclk = (os_short)pin_get_prm(device->device_pin, PIN_SCLK);
        bus->spec.spi.bus_nr = device->device_pin->bank;

#if OSAL_DEBUG
        os_strncpy(buf, "SPI bus init: ", sizeof(buf));

        os_strncat(buf, "bus_nr=", sizeof(buf));
        osal_int_to_str(nbuf, sizeof(nbuf), bus->spec.spi.bus_nr);
        os_strncat(buf, nbuf, sizeof(buf));

        os_strncat(buf, ", miso=", sizeof(buf));
        osal_int_to_str(nbuf, sizeof(nbuf), bus->spec.spi.miso);
        os_strncat(buf, nbuf, sizeof(buf));

        os_strncat(buf, ", mosi=", sizeof(buf));
        osal_int_to_str(nbuf, sizeof(nbuf), bus->spec.spi.mosi);
        os_strncat(buf, nbuf, sizeof(buf));

        os_strncat(buf, ", sclk=", sizeof(buf));
        osal_int_to_str(nbuf, sizeof(nbuf), bus->spec.spi.sclk);
        os_strncat(buf, nbuf, sizeof(buf));

        osal_info("pins", OSAL_SUCCESS, buf);
#endif
        /* Here we could set SPI speed for the current_device and enable chip select for it.
         */
    }
#endif

#if PINS_I2C
    if (bus->bus_type == PINS_I2C_BUS)
    {
        /* Get GPIO pin numbers and bus number.
         */
        bus->spec.i2c.sda = (os_short)pin_get_prm(device->device_pin, PIN_SDA);
        bus->spec.i2c.scl = (os_short)pin_get_prm(device->device_pin, PIN_SCL);
        bus->spec.i2c.bus_nr = device->device_pin->bank;

#if OSAL_DEBUG
        os_strncpy(buf, "I2C bus init: ", sizeof(buf));

        os_strncat(buf, "bus_nr=", sizeof(buf));
        osal_int_to_str(nbuf, sizeof(nbuf), bus->spec.i2c.bus_nr);
        os_strncat(buf, nbuf, sizeof(buf));

        os_strncat(buf, ", sda=", sizeof(buf));
        osal_int_to_str(nbuf, sizeof(nbuf), bus->spec.i2c.sda);
        os_strncat(buf, nbuf, sizeof(buf));

        os_strncat(buf, ", scl=", sizeof(buf));
        osal_int_to_str(nbuf, sizeof(nbuf), bus->spec.i2c.scl);
        os_strncat(buf, nbuf, sizeof(buf));

        osal_info("pins", OSAL_SUCCESS, buf);
#endif

    }
#endif
}


/**
****************************************************************************************************

  @brief Platform specific SPI/I2C device initialization.
  @anchor pins_init_device

  The pins_init_device() function initializes a SPI/I2C device for platform.

  pigpio example:

    bbSPIOpen(10, MISO, MOSI, SCLK, 10000, 0); // device 1
    bbSPIOpen(11, MISO, MOSI, SCLK, 20000, 3); // device 2

  @param   device Pointer to device structure.
  @param   prm Device parameters.
  @return  None.

****************************************************************************************************
*/
void pins_init_device(
    struct PinsBusDevice *device,
    struct PinsBusDeviceParams *prm)
{
    PinsBus *bus;
#if OSAL_DEBUG
    os_char buf[128], nbuf[OSAL_NBUF_SZ];
#endif

    bus = device->bus;

    /* Clear bus type specific variables for the device.
     */
    os_memclear(&device->spec, sizeof (PinsDeviceVariables));

#if PINS_SPI
    if (bus->bus_type == PINS_SPI_BUS)
    {
        /* Get GPIO chip select pin number, baud, flags and optional device number.
         */
        device->spec.spi.cs = (os_short)pin_get_prm(device->device_pin, PIN_CS);
        device->spec.spi.bus_frequency = (os_uint)pin_get_frequency(device->device_pin, 20000);
        device->spec.spi.flags = (os_ushort)pin_get_prm(device->device_pin, PIN_FLAGS);
        device->spec.spi.device_nr = device->device_pin->addr;

#if OSAL_DEBUG
        os_strncpy(buf, "SPI device init: ", sizeof(buf));

        os_strncat(buf, "device_nr=", sizeof(buf));
        osal_int_to_str(nbuf, sizeof(nbuf), device->spec.spi.device_nr);
        os_strncat(buf, nbuf, sizeof(buf));

        os_strncat(buf, ", bus_nr=", sizeof(buf));
        osal_int_to_str(nbuf, sizeof(nbuf), bus->spec.spi.bus_nr);
        os_strncat(buf, nbuf, sizeof(buf));

        os_strncat(buf, ", miso=", sizeof(buf));
        osal_int_to_str(nbuf, sizeof(nbuf), bus->spec.spi.miso);
        os_strncat(buf, nbuf, sizeof(buf));

        os_strncat(buf, ", mosi=", sizeof(buf));
        osal_int_to_str(nbuf, sizeof(nbuf), bus->spec.spi.mosi);
        os_strncat(buf, nbuf, sizeof(buf));

        os_strncat(buf, ", sclk=", sizeof(buf));
        osal_int_to_str(nbuf, sizeof(nbuf), bus->spec.spi.sclk);
        os_strncat(buf, nbuf, sizeof(buf));

        os_strncat(buf, ", cs=", sizeof(buf));
        osal_int_to_str(nbuf, sizeof(nbuf), device->spec.spi.cs);
        os_strncat(buf, nbuf, sizeof(buf));

        os_strncat(buf, ", frequency=", sizeof(buf));
        osal_int_to_str(nbuf, sizeof(nbuf), device->spec.spi.bus_frequency);
        os_strncat(buf, nbuf, sizeof(buf));

        os_strncat(buf, ", flags=", sizeof(buf));
        osal_int_to_str(nbuf, sizeof(nbuf), device->spec.spi.flags);
        os_strncat(buf, nbuf, sizeof(buf));

        osal_info("pins", OSAL_SUCCESS, buf);
#endif
        /* Here we could set SPI speed for the current_device and enable chip select for it.
         */
    }
#endif

#if PINS_I2C
    if (bus->bus_type == PINS_I2C_BUS)
    {
        /* Get flags and device number.
         */
        device->spec.i2c.flags = (os_ushort)pin_get_prm(device->device_pin, PIN_FLAGS);
        device->spec.i2c.device_nr = device->device_pin->addr;

#if OSAL_DEBUG
        os_strncpy(buf, "I2C device init: ", sizeof(buf));

        os_strncat(buf, "device_nr=", sizeof(buf));
        osal_int_to_str(nbuf, sizeof(nbuf), device->spec.i2c.device_nr);
        os_strncat(buf, nbuf, sizeof(buf));

        os_strncat(buf, "bus_nr=", sizeof(buf));
        osal_int_to_str(nbuf, sizeof(nbuf), bus->spec.i2c.bus_nr);
        os_strncat(buf, nbuf, sizeof(buf));

        os_strncat(buf, ", sda=", sizeof(buf));
        osal_int_to_str(nbuf, sizeof(nbuf), bus->spec.i2c.sda);
        os_strncat(buf, nbuf, sizeof(buf));

        os_strncat(buf, ", scl=", sizeof(buf));
        osal_int_to_str(nbuf, sizeof(nbuf), bus->spec.i2c.scl);
        os_strncat(buf, nbuf, sizeof(buf));

        os_strncat(buf, ", flags=", sizeof(buf));
        osal_int_to_str(nbuf, sizeof(nbuf), device->spec.i2c.flags);
        os_strncat(buf, nbuf, sizeof(buf));

        osal_info("pins", OSAL_SUCCESS, buf);

        if (bus->spec.i2c.bus_nr) {
            if (bus->spec.i2c.sda != 2 ||
                bus->spec.i2c.scl != 3)
            {
                osal_debug_error("Wrong I2C bus 1 pins.");
                osal_debug_error("Must be: sda=2, scl=3.");
            }
        }
        else {
            if (bus->spec.i2c.sda != 0 ||
                bus->spec.i2c.scl != 1)
            {
                osal_debug_error("Wrong I2C bus 0 pins.");
                osal_debug_error("Must be: sda=0, scl=1.");
            }
        }
#endif
    }
#endif
}


/**
****************************************************************************************************

  @brief Close a specific SPI/I2C device.
  @anchor pins_close_device

  The pins_close_device() function...

  @param   device Pointer to device structure.
  @return  None.

****************************************************************************************************
*/
void pins_close_device(
    struct PinsBusDevice *device)
{
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
    osalStatus s = OSAL_STATUS_NOT_SUPPORTED;

    bus = pins_devicebus.current_bus;

#if PINS_SPI
    if (bus->bus_type == PINS_SPI_BUS) {
        s = pins_bus_run_spi(bus);
    }
#endif
#if PINS_I2C
    if (bus->bus_type == PINS_I2C_BUS) {
        s = pins_bus_run_i2c(bus);
    }
#endif

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

    osal_thread_set_priority(OSAL_THREAD_PRIORITY_NORMAL);

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
static long ulledoo; if (++ulledoo > 10009) {osal_debug_error("ulledoo SPI\n"); ulledoo = 0;}
        }
    }
#endif

#if PINS_I2C
    if (bus->bus_type == PINS_I2C_BUS)
    {
        /* Run the the device bus, until program or SPI/I2C communication is
           to be terminated.
         */
        while (osal_go() && !pins_devicebus.terminate)
        {
            s = pins_bus_run_i2c(bus);
            if (s == OSAL_COMPLETED) {
                os_timeslice();
            }
static long ulledoo; if (++ulledoo > 10009) {osal_debug_error("ulledoo I2C\n"); ulledoo = 0;}
        }
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
    OSAL_UNUSED(flags);

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

/* OSAL_MULTITHREAD_SUPPORT */
#endif


#if PINS_SPI
/**
****************************************************************************************************

   @brief Send data to SPI bus and receive reply.
   @anchor pins_spi_transfer

   The pins_spi_transfer() function sends a message to current SPI device and gets a reply.
   If multiple messages are used with the device, gen_req_func() and proc_resp_func()
   functions process one of these at the time.

   @param   device Pointer to SPI/I2C device structure.
   @return  OSAL_COMPLETED if this was the last IO message to this device. OSAL_SUCCESS otherwise.
            OSAL_PENDING = move to next device.

****************************************************************************************************
*/
static osalStatus pins_spi_transfer(
    PinsBusDevice *device)
{
    device->gen_req_func(device);
    /* spi transfer here */
    device->proc_resp_func(device);
    return OSAL_PENDING;
}


/**
****************************************************************************************************

   @brief Send one SPI bus request and receive reply.
   @anchor pins_do_spi_bus_transaction

   The pins_do_spi_bus_transaction() function sends buffer content to SPI bus gets a reply.
   Here we give turn to every SPI device and every message for the device. But one call
   to this function transfers only one request/reply pair.

   @param   device Pointer to SPI/I2C device structure.
   @return  OSAL_COMPLETED if this was the last IO message to this of the last IO device
            in the bus. OSAL_SUCCESS otherwise.

****************************************************************************************************
*/
static osalStatus pins_bus_run_spi(
    PinsBus *bus)
{
    PinsBusDevice *current_device;
    osalStatus s, final_s = OSAL_SUCCESS;

    current_device = bus->current_device;

    s = pins_spi_transfer(current_device);

    /* Move on to the next device ?
     */
    if (s != OSAL_SUCCESS) {
        /* Change chip select, if we have more than 1 device.
         */

        current_device = current_device->next_device;
        if (current_device == OS_NULL) {
            current_device = bus->first_bus_device;
            final_s = OSAL_COMPLETED;
        }

        bus->current_device = current_device;
    }

    return final_s;
}

/* PINS_SPI */
#endif


#if PINS_I2C
/**
****************************************************************************************************

   @brief Send data to I2C bus and receive reply.
   @anchor pins_i2c_transfer

   The pins_i2c_transfer() function sends a message to current SPI or I2C device and
   gets a reply. If multiple messages are used with the device, gen_req_func() and
   proc_resp_func() functions process one of these at the time.

   @param   device Pointer to I2C device structure.
   @return  OSAL_COMPLETED if this was the last IO message to this device. OSAL_SUCCESS otherwise.
            OSAL_PENDING = move to next device.

****************************************************************************************************
*/
static osalStatus pins_i2c_transfer(
    PinsBusDevice *device)
{
    device->gen_req_func(device);
    device->proc_resp_func(device);
    return OSAL_PENDING;
}


/**
****************************************************************************************************

   @brief Send one I2C bus request and receive reply.
   @anchor pins_do_i2c_bus_transaction

   The pins_do_i2c_bus_transaction() function sends buffer content to SPI bus gets a reply.
   Here we give turn to every SPI/I2C device and every message for the device. But one call
   to this function transfers only one request/reply pair.

   @param   device Pointer to I2C device structure.
   @return  OSAL_COMPLETED if this was the last IO message to this of the last IO device
            in the bus. OSAL_SUCCESS otherwise.

****************************************************************************************************
*/
static osalStatus pins_bus_run_i2c(
    PinsBus *bus)
{
    PinsBusDevice *current_device;
    osalStatus s, final_s = OSAL_SUCCESS;

    current_device = bus->current_device;

    s = pins_i2c_transfer(current_device);

    /* Move on to the next device ?
     */
    if (s != OSAL_SUCCESS) {
        current_device = current_device->next_device;
        if (current_device == OS_NULL) {
            current_device = bus->first_bus_device;
            final_s = OSAL_COMPLETED;
        }

        bus->current_device = current_device;
    }

    return final_s;
}

/* PINS_I2C */
#endif

#endif
#endif
