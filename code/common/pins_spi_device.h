/**

  @file    common/pins_spi_device.h
  @brief   SPI device management.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    25.7.2020

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

struct Pin;
struct pinsSpiBus;
struct pinsSpiDevice;


typedef struct pinsSpi
{
    struct pinsSpiBus *first_bus;
}
pinsSpi;


typedef struct pinsSpiBus
{
    const struct Pin *miso;
    const struct Pin *mosi;
    const struct Pin *clock;

    struct pinsSpiBus *next_bus;
    struct pinsSpiDevice *first_device_on_bus;
}
pinsSpiBus;


typedef struct pinsSpiDevice
{
    const struct Pin **data_pins;
    os_int n_data_pins;

    /** Chip select pin.
     */
    const struct Pin *cs;

    /** Chip select pin.
     */
    int bitrate;

    /** Next SPI device in the same SPI bus.
     */
    struct pinsSpiDevice *next_device_on_bus;
}
pinsSpiDevice;

typedef struct pinsSpiParams
{
    /** Chip select pin.
     */
    int bitrate;
}
pinsSpiParams;

typedef void pins_spi_device_func(
    pinsSpiBus *bus,
    pinsSpiDevice *spi_device);

typedef void pins_spi_callback(
    pinsSpiDevice *spi_device);

void pins_initialize_spi(
    pinsSpi *spi);

void pins_run_spi(
    pinsSpi *spi);

void pins_run_spi_bus(
    pinsSpiBuf *spi_bus);

void pins_add_spi_device(
    pinsSpi *spi,
    const struct Pin *miso,
    const struct Pin *mosi,
    const struct Pin *clock,
    const struct Pin *cs,
    const struct Pin **data_pins,
    os_int n_data_pins,
    pins_spi_device_func *func,
    pinsSpiParams *prm);

#endif
