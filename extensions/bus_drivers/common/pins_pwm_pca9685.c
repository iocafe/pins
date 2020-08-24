/**

  @file    extensions/bus_drivers/common/pins_adc_pca9685.c
  @brief   Driver for PCA9685 12-Channel PWM I2C chip
  @author  Pekka Lehtikoski
  @version 1.0
  @date    22.8.2020

  PCA9685 is 16-Channel (12-bit) PWM/servo driver with I2C interface

  - I2C-controlled PWM driver with a built in clock.
  - 5V compliant, which means you can control it from a 3.3V microcontroller and still
    safely drive up to 6V outputs.
  - 6 address select pins so you can wire up to 62 of these on a single i2c bus.
  - Adjustable frequency PWM up to about 1.6 KHz.
  - 12-bit resolution for each output - for servos, that means about 4us resolution
    at 60Hz update rate.
  - Configurable push-pull or open-drain output.

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

static PinsMcp3208Ext pca9685_ext[PINS_MAX_MCP3208_ADC];
static os_short pca9685_nro_chips;

/**
****************************************************************************************************

   @brief Initialize driver
   @anchor pca9685_initialize_driver

   pca9685_initialize_driver() function initializes global variables for bus device driver.
   @return  None.

****************************************************************************************************
*/
void pca9685_initialize_driver()
{
    pca9685_nro_chips = 0;
    os_memclear(pca9685_ext, sizeof(pca9685_ext));
}


/**
****************************************************************************************************

   @brief Initialize device
   @anchor initialize_pca9685

   The pca9685_initialize() function initializes a bus device structure for a specific
   MCP3208 chip.

   @param   device Structure representing SPI device.
   @return  None.

****************************************************************************************************
*/
void pca9685_initialize(struct PinsBusDevice *device)
{
    PinsBusDeviceParams prm;
    os_short *adc_value, i;

    if (pca9685_nro_chips >= PINS_MAX_MCP3208_ADC) {
        osal_debug_error("Reserved number of MCP3208 chip exceeded in JSON, increase PINS_MAX_MCP3208_ADC");
        return;
    }

    device->ext = &pca9685_ext[pca9685_nro_chips++];
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

   @brief Prepare request to send
   @anchor pca9685_gen_req

   The pca9685_gen_req() function prepares the next request to send to the device into buffer
   within the bus struture.

   @param   device Structure representing SPI device.
   @return  None.

****************************************************************************************************
*/
void pca9685_gen_req(struct PinsBusDevice *device)
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
}


/**
****************************************************************************************************

   @brief Process reply from SPI device
   @anchor pca9685_proc_resp

   The pca9685_proc_resp() function processed the received reply from buffer
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
osalStatus pca9685_proc_resp(struct PinsBusDevice *device)
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
   @anchor pca9685_set

   The pca9685_set() function is not needed for ADC, it is read only.

   @param   device Structure representing SPI device.
   @param   addr ADC channel 0 ... 7.
   @param   value Value to set, ignored.
   @return  None

****************************************************************************************************
*/
void pca9685_set(struct PinsBusDevice *device, os_short addr, os_int value)
{
    OSAL_UNUSED(device);
    OSAL_UNUSED(addr);
    OSAL_UNUSED(value);
}


/**
****************************************************************************************************

   @brief Get SPI device data
   @anchor pca9685_get

   The pca9685_get() function reads ASC channel value received from the device.

   @param   device Structure representing SPI device.
   @param   addr ADC channel 0 ... 7.
   @return  value ADC value received 0 ... 4095. -1 if none read.

****************************************************************************************************
*/
os_int pca9685_get(struct PinsBusDevice *device, os_short addr)
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

