/**

  @file    extensions/spi/common/pins_devicebus.h
  @brief   SPI and I2C.
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
#pragma once
#ifndef PINS_SPI_H_
#define PINS_SPI_H_
#include "pinsx.h"
#if PINS_SPI || PINS_I2C

struct PinsBusDevice;
struct PinsBus;

/** Supported bus types.
 */
typedef enum {
    PINS_SPI_BUS = 1,
    PINS_I2C_BUS = 2
}
pinsBusType;

/** SPI device callback, when we can send request to device.
 */
typedef void pinsGenerateDeviceRequest(
    struct PinsBusDevice *device);

/** SPI device callback, when reply is received from the device.
 */
typedef osalStatus pinsProcessDeviceResponce(
    struct PinsBusDevice *device);

/** Function type to set value to SPI or I2C device driver. Implemented by driver.
 */
typedef void pinsBusSet(
    struct PinsBusDevice *device,
    os_short addr,
    os_int value);

/** Function type to get value from SPI or I2C device driver. Implemented by driver.
 */
typedef os_int pinsBusGet(
    struct PinsBusDevice *device,
    os_short addr);


typedef struct PinsSpiDeviceVariables
{
    /** Chip select pin. Typically active LOW.
     */
    os_short cs;

    /** SPI device number or "addr". This can be zero if not needed.
     */
    os_short device_nr;

    /** SPI bus flags.
     */
    os_ushort flags;

    /** SPI device handle
     */
    os_int handle;

    /** Bus speed (baud) for this device.
     */
    os_uint bus_frequency;

    /** Repeating transer error reported, to not repeat.
     */
    os_boolean error_reported;
}
PinsSpiDeviceVariables;

typedef struct PinsI2cDeviceVariables
{
    /** I2C device number or "addr".
     */
    os_short device_nr;

    /** I2C bus flags.
     */
    os_ushort flags;
}
PinsI2cDeviceVariables;

typedef union {
    PinsSpiDeviceVariables spi;
    PinsI2cDeviceVariables i2c;
}
PinsDeviceVariables;



/** Structure representing either a SPI or I2C device.
 */
typedef struct PinsBusDevice
{
    /** Pointer to pin structure representing the device. This must be the first item
        in the structure: The structure is initialization code is generated from JSON.
     */
    const struct Pin *device_pin;

    /** Pointer to SPI bus to which this device is connected. Must be the second item
        in the structure.
     */
    struct PinsBus *bus;

    /** Netx in linked list of SPI or I2C devices, must not be modified when SPI is running.
        Must be the third item in the structure.
     */
    struct PinsBusDevice *next_device;

    /** Driver function pointers. Must be fourth to seventh items in the structure.
     */
    pinsGenerateDeviceRequest *gen_req_func;
    pinsProcessDeviceResponce *proc_resp_func;
    pinsBusSet *set_func;
    pinsBusGet *get_func;

    /** Enable device flag. Devices can be disabled if not connected,
        or to speed up communication to other device in the bus.
     */
    // os_boolean enable;

    /** Bus type specific variables.
     */
    PinsDeviceVariables spec;

    /* Extended device data structure */
    void *ext;
}
PinsBusDevice;


/** SPI message buffer size, bytes.
 */
#define PINS_BUS_BUF_SZ 32


typedef struct PinsSpiBusVariables
{
    /** Bus pins.
     */
    os_short miso, mosi, sclk;

    /** SPI bus number or "bank". This can be zero if not needed.
     */
    os_short bus_nr;
}
PinsSpiBusVariables;

typedef struct PinsI2cBusVariables
{
    /** Bus pins.
     */
    os_short sda, scl;

    /** i2c bus number or "bank".
     */
    os_short bus_nr;
}
PinsI2cBusVariables;

/* Bus type specific union.
 */
typedef union {
    PinsSpiBusVariables spi;
    PinsI2cBusVariables i2c;
}
PinsBusVariables;


/**
 */
typedef struct PinsBus
{
    /** Either PINS_SPI_BUS or PINS_I2C_BUS. This must be the first item
        in the structure: The structure is initialization code is generated from JSON.
     */
    pinsBusType bus_type;

    /** Linked list of SPI devices, must not be modified when SPI is running.
        Must be second item of the structure.
     */
    PinsBusDevice *first_bus_device;

    /** Linked list of SPI buses, must not be modified when SPI is running.
        Must be third item of the structure.
     */
    struct PinsBus *next_bus;

    /** Current device (which device has the turn).
     */
    PinsBusDevice *current_device;

    /** Bus type specific variables.
     */
    PinsBusVariables spec;

    /** SPI message buffer, for outgoing and incoming messages.
     */
    os_uchar outbuf[PINS_BUS_BUF_SZ], inbuf[PINS_BUS_BUF_SZ];

    /** Number of bytes in buffer.
     */
    os_short outbuf_n, inbuf_n;
}
PinsBus;


/** SPI state
 */
typedef struct PinsDeviceBus
{
    /** This must be first item, intialized from JSON generated code.
     */
    PinsBus *first_bus;

    /** Current bus in signle thread mode.
     */
    PinsBus *current_bus;

#if OSAL_MULTITHREAD_SUPPORT
    /** Number of threads running.
     */
    volatile os_short thread_count;

    /** Request to terminate devicebus.
     */
    volatile os_boolean terminate;
#endif
}
PinsDeviceBus;

/* Parameters fron driver to operating specific bus initialization.
 */
typedef struct PinsBusDeviceParams
{
    os_int extra_parameters_here;
}
PinsBusDeviceParams;

/* Global device bus main structure.
 */
extern PinsDeviceBus pins_devicebus;

void pins_init_bus(
    PinsBus *bus);

void pins_init_device(
    struct PinsBusDevice *device,
    struct PinsBusDeviceParams *prm);

void pins_close_device(
    struct PinsBusDevice *device);


/* Single threaded use. Call from main loop to run device bus.
 */
void pins_run_devicebus(
    os_int flags);

#if OSAL_MULTITHREAD_SUPPORT

/* Run multi threaded device bus. The function starts thread for each SPI bus.
 */
void pins_start_multithread_devicebus(
    os_int flags);

void pins_stop_multithread_devicebus(
    void);

#endif




/*
void pins_set_spi_bus_speed(
    PinsBus *spi_bus,
    os_int speed);

void pins_send_spi_request(
    PinsBus *spi_bus,
    const os_uchar buf,
    os_short buf_sz);
 */


#endif
#endif
