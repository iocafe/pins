/**

  @file    extensions/bus_drivers/common/pins_adc_mcp3208.c
  @brief   Driver for MCP3208 8-Channel 12-Bit A/D converter
  @author  Pekka Lehtikoski
  @version 1.0
  @date    16.8.2020

  MCP3208 is 8-Channel 12-Bit A/D converters chip created by Microchip Technology Inc, which
  connects to the microcontroller trough SPI bus. This chip is nice for hobbyist, it is simple
  to program and is available as 16-PDIP (pins for trough holes) which is perfect for breadboard
  or home made PCB prototyping. There is also 4 channel version of the chip, MCP3204.

  - Device are successive approximation 12-bit Analogto-Digital (A/D) Converters with on-board
    sample and hold circuitry.
  - The MCP3208 is programmable to provide four pseudo-differential input pairs or eight
    single-ended inputs.
  - Single supply operation: 2.7V - 5.5V
  - 100 ksps max. sampling rate at V DD = 5V, 50 ksps max. sampling rate at V DD = 2.7V

  pins_io.json (raspberry):

  {
    "io": [{
      "groups": [
        {
          "name": "analog_inputs",
          "pins": [
            {"name": "sig0", "device": "spi.adc1", "addr": 0, "max": 4095},
            {"name": "sig1", "device": "spi.adc1", "addr": 1, "max": 4095},
            {"name": "sig4", "device": "spi.adc1", "addr": 3, "max": 4095}
          ]
        },
        {
          "name": "spi",
          "pins": [
            {"name": "adc1", "driver":"mcp3208", "bank": 0, "addr":0, "miso": 9, "mosi": 10,
             "sclk": 11, "cs": 8, "frequency-kHz": 1000, "flags": 3}
          ]
        }
      ]
    }]
  }

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#ifndef PINS_MAX_MCP3208_ADC
#define PINS_MAX_MCP3208_ADC 2
#endif

#include "pinsx.h"
#if PINS_SPI
#if PINS_MAX_MCP3208_ADC

#define MCP3208_NRO_ADC_CHANNELS 8

typedef struct PinsMcp3208Ext
{
    os_short adc_value[MCP3208_NRO_ADC_CHANNELS];
    os_uchar current_ch;
    os_uchar reserved;
}
PinsMcp3208Ext;

static PinsMcp3208Ext mcp3208_ext[PINS_MAX_MCP3208_ADC];
static os_short mcp3208_nro_chips;

/**
****************************************************************************************************

   @brief Initialize driver
   @anchor mcp3208_initialize_driver

   mcp3208_initialize_driver() function initializes global variables for bus device driver.
   @return  None.

****************************************************************************************************
*/
void mcp3208_initialize_driver()
{
    mcp3208_nro_chips = 0;
    os_memclear(mcp3208_ext, sizeof(mcp3208_ext));
}


/**
****************************************************************************************************

   @brief Initialize device
   @anchor mcp3208_initialize_device

   The mcp3208_initialize_device() function initializes a bus device structure for a specific
   MCP3208 chip.

   @param   device Structure representing SPI device.
   @return  None.

****************************************************************************************************
*/
void mcp3208_initialize_device(struct PinsBusDevice *device)
{
    PinsBusDeviceParams prm;
    os_short *adc_value, i;

    if (mcp3208_nro_chips >= PINS_MAX_MCP3208_ADC) {
        osal_debug_error("Reserved number of MCP3208 chip exceeded in JSON, increase PINS_MAX_MCP3208_ADC");
        return;
    }

    device->ext = &mcp3208_ext[mcp3208_nro_chips++];
    adc_value = ((PinsMcp3208Ext*)(device->ext))->adc_value;
    for (i = 0; i < MCP3208_NRO_ADC_CHANNELS; i++) {
        adc_value[i] = -1;
    }

    /* Call platform specific device initialization.
     */
    os_memclear(&prm, sizeof(prm));
    pins_init_device(device, &prm);
}


/**
****************************************************************************************************

   @brief Initialize "pin" of bus device.
   @anchor mcp3208_initialize_pin

   The mcp3208_initialize_pin() function initializes a bus device's pin. In practice this
   function may set some data in device strucure.

   @param   pin Structure representing bus device "pin".
   @return  None.

****************************************************************************************************
*/
void mcp3208_initialize_pin(const struct Pin *pin)
{
}


/**
****************************************************************************************************

   @brief Prepare request to send
   @anchor mcp3208_gen_req

   The mcp3208_gen_req() function prepares the next request to send to the device into buffer
   within the bus struture.

   @param   device Structure representing SPI device.
   @return  Always OSAL_SUCCESS. Device change checking is done when processing reply.

****************************************************************************************************
*/
osalStatus mcp3208_gen_req(struct PinsBusDevice *device)
{
    PinsMcp3208Ext *ext;
    os_uchar *buf, current_ch;

    osal_debug_assert(device->bus != OS_NULL);
    buf = device->bus->outbuf;
    ext = (PinsMcp3208Ext*)device->ext;
    current_ch = ext->current_ch;

    buf[0] = 0x06 | ((current_ch & 0x04) >> 2);
    buf[1] = (os_uchar)((current_ch & 0x03) << 6);
    buf[2] = 0;
    device->bus->inbuf_n = device->bus->outbuf_n = 3;
    return OSAL_SUCCESS;
}


/**
****************************************************************************************************

   @brief Process reply from SPI device
   @anchor mcp3208_proc_resp

   The mcp3208_proc_resp() function processed the received reply from buffer
   within the bus struture. It stores ADC value for the channel for the device

   Note: Sensibility checks for replay should be added, plus some kind of error counter would
   be appropriate to know if the design is failing.

   @param   device Structure representing SPI device.
   @return  OSAL_COMPLETED indicates that this was last SPI transaction needed for this device
            so that all data has been transferred from device. Value OSAL_SUCCESS to indicates
            that there is more to read. Other values indicate that SPI reply was not
            recieved or was errornous.

****************************************************************************************************
*/
osalStatus mcp3208_proc_resp(struct PinsBusDevice *device)
{
    PinsMcp3208Ext *ext;
    os_uchar *buf, current_ch;
    os_short *adc_value;

    buf = device->bus->inbuf;
    ext = (PinsMcp3208Ext*)device->ext;
    current_ch = ext->current_ch;
    adc_value = ext->adc_value;

    adc_value[current_ch] = (os_short)(((os_ushort)(buf[1] & 0x0F) << 8) | (os_ushort)buf[2]);

    if (++current_ch < PINS_MAX_MCP3208_ADC) {
        ext->current_ch = current_ch;
        return OSAL_SUCCESS;
    }

    ext->current_ch = 0;
    return OSAL_COMPLETED;
}


/**
****************************************************************************************************

   @brief Set data to SPI device
   @anchor mcp3208_set

   The mcp3208_set() function is not needed for ADC, it is read only.

   @param   device Structure representing SPI device.
   @param   addr ADC channel 0 ... 7.
   @param   value Value to set, ignored.
   @return  OSAL_STATUS if successfull. Other values indicate a hardware error, specifically
            OSAL_STATUS_NOT_CONNECTED if SPI device is not connected.

****************************************************************************************************
*/
osalStatus mcp3208_set(struct PinsBusDevice *device, os_short addr, os_int value)
{
    OSAL_UNUSED(device);
    OSAL_UNUSED(addr);
    OSAL_UNUSED(value);
    return OSAL_SUCCESS;
}


/**
****************************************************************************************************

   @brief Get SPI device data
   @anchor mcp3208_get

   The mcp3208_get() function reads ASC channel value received from the device.

   @param   device Structure representing SPI device.
   @param   addr ADC channel 0 ... 7.
   @return  value ADC value received 0 ... 4095. -1 if none read.

****************************************************************************************************
*/
os_int mcp3208_get(struct PinsBusDevice *device, os_short addr)
{
    os_short *adc_value;

    if (addr < 0 || addr >= MCP3208_NRO_ADC_CHANNELS) {
        return -1;
    }

    adc_value = ((PinsMcp3208Ext*)(device->ext))->adc_value;
    return adc_value[addr];
}

#endif
#endif

