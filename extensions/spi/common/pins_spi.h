/**

  @file    extensions/spi/common/pins_spi.h
  @brief   Camera hardware API.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    16.8.2020

  - Run sequentially. Set speed and chip select pin. Send request. Wait for reply.
    Get reply and process it. The pinsGenerateSpiRequest() is called to generate the request
    and pinsProcessSpiResponce() to process it. Main loop calls this for every SPI device in turn.
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
#pragma once
#ifndef PINS_SPI_H_
#define PINS_SPI_H_
#include "pinsx.h"


struct pinsSpiDevice;
struct pinsSpiBus;


/** SPI device callback, when we can send eequest to device.
 */
typedef void pinsGenerateSpiRequest(
    struct pinsSpiDevice *spi_device,
    void *context);

/** SPI device callback, when reply is received from the device.
 */
typedef void pinsProcessSpiResponce(
    struct pinsSpiDevice *spi_device,
    void *context);


/** Camera photo as received by camera callback function.
 */
typedef struct pinsSpiDevice
{

    /** Pointer to chip select pin structure.
     */
    const struct Pin *cs_pin;

    os_int bus_speed;

    // os_boolean enable;

    pinsGenerateSpiRequest *gen_req_func;
    pinsProcessSpiResponce *proc_resp_func;


    /** Pointer to SPI bus to which this device is connected.
     */
    struct pinsSpiBus *spi_bus;

    /** Netx in linked list of SPI devices, must not be modified when SPI is running.
     */
    struct pinsSpiDevice *next_spi_device;
}
pinsSpiDevice;


/**
 */
typedef struct pinsSpiBus
{

    /** Pointer to pin structure.
     */
    const struct Pin *miso_pin;

    /**
     */
    const struct Pin *mosi_pin;

    /**
     */
    const struct Pin *clock_pin;

    /* Linked list of SPI devices, must not be modified when SPI is running.
     */
    pinsSpiDevice *first_spi_device;

    /* Linked list of SPI buses, must not be modified when SPI is running.
     */
    struct pinsSpiBus *next_spi_bus;
}
pinsSpiBus;


/** SPI state
 */
typedef struct pinsSpi
{
    pinsSpiBus *first_bus;
}
pinsSpi;


/*
void pins_set_spi_bus_speed(
    pinsSpiDevice *spi_device);

void pins_send_spi_request(
    pinsSpiDevice *spi_device,
    const os_uchar buf,
    os_int buf_sz); */


#endif
