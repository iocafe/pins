/**

  @file    extensions/spi/spi_collection/pins_adc_mcp3208.c
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

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "pinsx.h"
#if PINS_ENABLE_ADC_MCP3208

/**
****************************************************************************************************

   @brief Store a photo as a "brick" within brick buffer for communication.
   @anchor pins_store_photo_as_brick

   The pins_store_photo_as_brick() function...

   @param   spi_device.
   @param   b Pointer to brick buffer into which to store the photo as "brick".
   @param   compression Should photo be compressed by this function and is so how?
   @return  Number of bytes. 0 if nothing to send.

****************************************************************************************************
*/

static os_short pins_adc_mcp3208_generate_spi_request(
    struct pinsSpiDevice *spi_device,
    void *context)
{
  unsigned char buff[3];
  int adcValue = 0;

  buff[0] = 0x06 | ((adc_channel & 0x04) >> 2);
  buff[1] = ((adc_channel & 0x03) << 6);
  buff[2] = 0;
  return OSAL_SUCCESS;
}


typedef void pinsProcessSpiResponce(
    struct pinsSpiDevice *spi_device,
    void *context)
{
     buff[1] = 0x0F & buff[1];
     adcValue = ( buff[1] << 8) | buff[2];

    //  digitalWrite(CS_MCP3208, 1);  // High : CS Inactive

    return adcValue;
}

#if -
{
  unsigned char buff[3];
  int adcValue = 0;

  buff[0] = 0x06 | ((adc_channel & 0x04) >> 2);
  buff[1] = ((adc_channel & 0x03) << 6);
  buff[2] = 0;

//  digitalWrite(CS_MCP3208, 0);  // Low : CS Active

  wiringPiSPIDataRW(SPI_CHANNEL, buff, 3);

  buff[1] = 0x0F & buff[1];
  adcValue = ( buff[1] << 8) | buff[2];

//  digitalWrite(CS_MCP3208, 1);  // High : CS Inactive

  return adcValue;
}

void add_adc_mcp

#endif

#endif
