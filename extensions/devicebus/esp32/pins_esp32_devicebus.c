/**

  @file    devicebus/esp32/pins_esp32_devicebus.c
  @brief   SPI and I2C for ESP32.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    21.3.2026

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "pinsx.h"
#ifdef OSAL_ESP32

#if PINS_SPI 
    #include "driver/spi_common.h"
    #include "driver/spi_master.h"

    /* Forward referred static functions.
     */
    static osalStatus pins_spi_transfer(
        PinsBusDevice *device);

    static osalStatus pins_bus_run_spi(
        PinsBus *bus);
#endif

#if PINS_I2C
    #include "driver/i2c_master.h"

    static osalStatus pins_i2c_transfer(
        PinsBusDevice *device);

    static osalStatus pins_bus_run_i2c(
        PinsBus *bus);
#endif

#if PINS_SPI || PINS_I2C

/**
****************************************************************************************************

  @brief Clear state variables in the SPI/I2C bus structure and initialize the bus.
  @anchor pins_init_bus

  The pins_init_bus() function initializes the SPI/I2C bus and clears old state data.
  (many microcontroller do not clear memory at soft reboot).

  @param   bus Pointer to bus structure.
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
    esp_err_t rval;

#if PINS_SPI
    spi_bus_config_t busconf;
#endif

#if PINS_I2C
    i2c_master_bus_config_t ic2bus_conf;
    i2c_master_bus_handle_t ic2bus_handle;
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
        /* Get GPIO pin numbers and bus number. Notice that SPI0 and SPI1 are
           reserved, we can use only SPI2 and SPI3.
         */
        bus->spec.spi.miso = (os_short)pin_get_prm(device->device_pin, PIN_MISO);
        bus->spec.spi.mosi = (os_short)pin_get_prm(device->device_pin, PIN_MOSI);
        bus->spec.spi.sclk = (os_short)pin_get_prm(device->device_pin, PIN_SCLK);
        bus->spec.spi.bus_nr = device->device_pin->bank ? SPI3_HOST : SPI2_HOST;

        /* Bus configuration debug printout.
         */
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
        /* Initialize the SPI bus .
         */
        os_memclear(&busconf, sizeof(busconf));
        busconf.mosi_io_num = bus->spec.spi.mosi;
        busconf.miso_io_num = bus->spec.spi.miso;
        busconf.sclk_io_num = bus->spec.spi.sclk;
        busconf.data0_io_num = busconf.data1_io_num = busconf.data2_io_num = busconf.data3_io_num = -1; 
        busconf.data4_io_num = busconf.data5_io_num = busconf.data6_io_num = busconf.data7_io_num = -1; 
        busconf.quadwp_io_num = busconf.quadhd_io_num = -1;
        busconf.max_transfer_sz = 32;
        
        rval = spi_bus_initialize((spi_host_device_t)bus->spec.spi.bus_nr,
            &busconf, SPI_DMA_DISABLED);
        ESP_ERROR_CHECK(rval);
        return;
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
        /* Setup i2c bus.
         */
        os_memclear(&ic2bus_conf, sizeof(ic2bus_conf));
        ic2bus_conf.i2c_port = -1;
        ic2bus_conf.sda_io_num = bus->spec.i2c.sda;
        ic2bus_conf.scl_io_num = bus->spec.i2c.scl;
        ic2bus_conf.clk_source = I2C_CLK_SRC_DEFAULT;
        // ic2bus_conf.enable_internal_pullup = 1;

        rval = i2c_new_master_bus(&ic2bus_conf, &ic2bus_handle);
        if (rval != ESP_OK) {
            osal_debug_error_int("i2c_new_master_bus() failed, rval=", rval);
            return;
        }

        bus->spec.i2c.handle.p = (void *)ic2bus_handle;
        return;
     }
#endif
}


/**
****************************************************************************************************

  @brief Platform specific SPI/I2C device initialization.
  @anchor pins_init_device

  The pins_init_device() function initializes a SPI/I2C device for platform.

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

#if PINS_SPI
    spi_device_interface_config_t devcfg;
    spi_device_handle_t handle;
#endif

#if PINS_I2C
    i2c_device_config_t i2c_config;
    i2c_master_dev_handle_t i2c_handle;
#endif

#if OSAL_DEBUG
    os_char buf[128], nbuf[OSAL_NBUF_SZ];
#endif
    esp_err_t rval;
    OSAL_UNUSED(prm);

    bus = device->bus;

    /* Clear bus type specific variables for the device.
     */
    os_memclear(&device->spec, sizeof (PinsDeviceVariables));

#if PINS_SPI
    if (bus->bus_type == PINS_SPI_BUS)
    {
        /* Get GPIO chip select pin number, baud, flags and optional device number.
         */
        device->spec.spi.cs = (os_short)pin_get_prm(device->device_pin, -1);
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

        if (device->spec.spi.bus_frequency < 32000 ||
            device->spec.spi.bus_frequency > 30000000)
        {
            osal_debug_error_int("SPI baud rate is outside 32k - 30M, setting",
                device->spec.spi.bus_frequency);
        }
#endif

        /* Add slave device 
         */
        os_memclear(&devcfg, sizeof(devcfg));
        devcfg.clock_speed_hz = device->spec.spi.bus_frequency;
        // devcfg.mode = 0,     //SPI mode 0 
        devcfg.spics_io_num = device->spec.spi.cs; // CS Pin 
        devcfg.queue_size = 1;
        devcfg.flags = SPI_DEVICE_HALFDUPLEX;
        // devcfg.pre_cb = NULL;
        // devcfg.post_cb = NULL;

        rval = spi_bus_add_device((spi_host_device_t)bus->spec.spi.bus_nr, &devcfg, &handle);
        if (rval != ESP_OK) {
            osal_debug_error_int("spi_bus_add_device() failed, rval=", rval);
            return;
        }            
        device->spec.spi.handle.p = (void*)handle;
        return;
    }
#endif

#if PINS_I2C
    if (bus->bus_type == PINS_I2C_BUS)
    {
        /* Get flags and device number.
         */
        device->spec.i2c.flags = (os_ushort)pin_get_prm(device->device_pin, PIN_FLAGS);
        device->spec.i2c.bus_frequency = (os_uint)pin_get_frequency(device->device_pin, 20000);
        device->spec.i2c.device_nr = device->device_pin->addr;

#if OSAL_DEBUG
        os_strncpy(buf, "I2C device init: ", sizeof(buf));

        os_strncat(buf, "device_nr=", sizeof(buf));
        osal_int_to_str(nbuf, sizeof(nbuf), device->spec.i2c.device_nr);
        os_strncat(buf, nbuf, sizeof(buf));

        os_strncat(buf, ", bus_nr=", sizeof(buf));
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

        os_strncat(buf, ", frequency=", sizeof(buf));
        osal_int_to_str(nbuf, sizeof(nbuf), device->spec.i2c.bus_frequency);
        os_strncat(buf, nbuf, sizeof(buf));

        osal_info("pins", OSAL_SUCCESS, buf);

        if (bus->spec.i2c.bus_nr != 1) {
            osal_debug_error("Warning, other than I2C bus 1 selected. The bus 0 is reserved for camera, etc.");
        }

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
        os_memclear(&i2c_config, sizeof(i2c_config));
        i2c_config.device_address = device->spec.i2c.device_nr;
        i2c_config.dev_addr_length = I2C_ADDR_BIT_LEN_7;
        i2c_config.scl_speed_hz = device->spec.i2c.bus_frequency;

        rval = i2c_master_bus_add_device((i2c_master_bus_handle_t)bus->spec.i2c.handle.p,&i2c_config, &i2c_handle);
        if (rval != ESP_OK) {
            osal_debug_error_int("i2c_master_bus_add_device() failed, rval=", rval);
            return;
        }            

        device->spec.i2c.handle.p = (void*)i2c_handle;
        return;
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
    PinsBus *bus;

    bus = device->bus;

#if PINS_SPI
    if (bus->bus_type == PINS_SPI_BUS)
    {
        if (device->spec.spi.handle.p != OS_NULL) { /* Was successfully opened? */
            ESP_ERROR_CHECK(spi_bus_remove_device((spi_device_handle_t)device->spec.spi.handle.p));
            device->spec.spi.handle.p = OS_NULL;
        }
        return;
    }
#endif

#if PINS_I2C
    if (bus->bus_type == PINS_I2C_BUS)
    {
        if (device->spec.i2c.handle.p != OS_NULL) {
            ESP_ERROR_CHECK(i2c_master_bus_rm_device(device->spec.i2c.handle.p));
        }
        return;
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
osalStatus pins_run_devicebus(
    os_int flags)
{
    PinsBus *bus;
    osalStatus s = OSAL_COMPLETED;
    OSAL_UNUSED(flags);

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

    return s;
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
        /* Run the the device bus, until program or SPI communication is
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
        /* Run the the device bus, until program or I2C communication is
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
    while (pins_devicebus.thread_count) {
        osal_sleep(50);
    }
}

#endif


#if PINS_SPI
/**
****************************************************************************************************

   @brief Send data to SPI or I2C bus and receive reply.
   @anchor pins_spi_transfer

   The pins_spi_transfer() function sends a message to current SPI and  gets a reply. 
   This function calls back SPI device specific application driver. The gen_req_func() callback
   is expected to generate a SPI request proc_resp_func() callback to process it.

   @param   device Pointer to SPI/I2C device structure.
   @return  OSAL_COMPLETED if this was the last IO message to this device. OSAL_SUCCESS otherwise.

****************************************************************************************************
*/
static osalStatus pins_spi_transfer(
    PinsBusDevice *device)
{
    PinsBus *bus;
    esp_err_t rval;
    osalStatus s;
    spi_transaction_t ta;

    bus = device->bus;

    /* If SPI device has not been successfully opened, print error and return OSAL_COMPLETED.
     */
    if (device->spec.spi.handle.p == OS_NULL) {
        if (!device->spec.spi.error_reported) {
            osal_debug_error_int("SPI device is not open, bus=", bus->spec.spi.bus_nr);
            device->spec.spi.error_reported = OS_TRUE;
        }
        return OSAL_COMPLETED;
    }

    /* Callback to SPI device specific to generate a request.
     */
    device->gen_req_func(device);

    /* Setup transaction 
     */
    os_memclear(&ta, sizeof(ta));
    ta.tx_buffer = bus->outbuf;
    ta.length = bus->outbuf_n;
    ta.rx_buffer = bus->inbuf;
    /* ta.rxlength: Total data length received, should be not greater
       than length in full - duplex mode(0 defaults this 
       to the value of length). */

    /* SPI data transfer.
     */
    rval = spi_device_polling_transmit(device->spec.spi.handle.p, &ta);
    if (rval != !device->spec.spi.error_reported) {
        osal_debug_error_int("SPI device is not open, bus=", bus->spec.spi.bus_nr);
        device->spec.spi.error_reported = OS_TRUE;
        bus->inbuf_n = 0;
        return OSAL_COMPLETED;
    }
    bus->inbuf_n = ta.rxlength;

    /* Callback to SPI device specific application driver to process the responce.
     */
    s = device->proc_resp_func(device);
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
    if (current_device == OS_NULL) {
        current_device = bus->first_bus_device;
        if (current_device == OS_NULL) {
            return OSAL_COMPLETED;
        }
    }

    s = pins_spi_transfer(current_device);

    /* Move on to the next device ?
     */
    if (s == OSAL_COMPLETED) {
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

****************************************************************************************************
*/
static osalStatus pins_i2c_transfer(
    PinsBusDevice *device)
{
#if 0
    PinsBus *bus;
    os_uchar *buf, *inbuf;
    osalStatus s;
    os_short n, i;
    int rval = -1;

    bus = device->bus;

    /* If I2C device has not been successfully opened, print error and return OSAL_COMPLETED.
     */
    if (device->spec.i2c.handle < 0) {
        if (!device->spec.i2c.error_reported) {
            osal_debug_error_int("i2c device is not open, bus=", bus->spec.i2c.bus_nr);
            device->spec.i2c.error_reported = OS_TRUE;
        }
        return OSAL_COMPLETED;
    }

    s = device->gen_req_func(device);

    switch (bus->spec.i2c.bus_operation)
    {
        case PINS_I2C_WRITE_BYTE_DATA:
            n = bus->outbuf_n;
            buf = bus->outbuf;
            for (i = 0; i < n; i+=2) {
                rval = i2cWriteByteData((unsigned)device->spec.i2c.handle, buf[i], buf[i+1]);
                if (rval) break;
            }

            if (rval) {
                if (!device->spec.i2c.error_reported) {
                    osal_debug_error_int("i2cWriteByteData failed on bus ", bus->spec.i2c.bus_nr);
                    device->spec.i2c.error_reported = OS_TRUE;
                }
                return OSAL_COMPLETED;
            }
            break;

        case PINS_I2C_READ_BYTE_DATA:
            n = bus->outbuf_n;
            buf = bus->outbuf;
            inbuf = bus->inbuf;

            for (i = 0; i < n; i++) {
                rval = i2cReadByteData((unsigned)device->spec.i2c.handle, buf[i]);
                if (rval < 0) break;
                inbuf[i] = (os_uchar)rval;
            }
            bus->inbuf_n = i;

            if (rval < 0) {
                if (!device->spec.i2c.error_reported) {
                    osal_debug_error_int("i2cReadByteData failed on bus ", bus->spec.i2c.bus_nr);
                    device->spec.i2c.error_reported = OS_TRUE;
                }
                return OSAL_COMPLETED;
            }

            s = device->proc_resp_func(device);
            break;
    }
    return s;
#endif
    return OSAL_SUCCESS;
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
    if (current_device == OS_NULL) {
        current_device = bus->first_bus_device;
        if (current_device == OS_NULL) {
            return OSAL_COMPLETED;
        }
    }

    s = pins_i2c_transfer(current_device);

    /* Move on to the next device ?
     */
    if (s == OSAL_COMPLETED) {
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

