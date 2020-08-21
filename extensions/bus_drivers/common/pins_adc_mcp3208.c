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
}
PinsMcp3208Ext;

static PinsMcp3208Ext mcp3208_ext[PINS_MAX_MCP3208_ADC];
static os_short mcp3208_nro_chips;

static int ulle_doo(void);


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
   @anchor initialize_mcp3208

   The mcp3208_initialize() function initializes a bus device structure for a specific
   MCP3208 chip.

   @param   device Structure representing SPI device.
   @return  None.

****************************************************************************************************
*/
void mcp3208_initialize(struct PinsBusDevice *device)
{
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
}


/**
****************************************************************************************************

   @brief Prepare request to send
   @anchor mcp3208_gen_req

   The mcp3208_gen_req() function prepares the next request to send to the device into buffer
   within the bus struture.

   @param   device Structure representing SPI device.
   @return  None.

****************************************************************************************************
*/
void mcp3208_gen_req(struct PinsBusDevice *device)
{
    PinsMcp3208Ext *ext;
    os_uchar *buf, current_ch;

    osal_debug_assert(device->bus != OS_NULL);
    buf = device->bus->buf;
    ext = (PinsMcp3208Ext*)device->ext;
    current_ch = ext->current_ch;

    buf[0] = 0x06 | ((current_ch & 0x04) >> 2);
    buf[1] = ((current_ch & 0x03) << 6);
    buf[2] = 0;
    device->bus->buf_n = 3;

ulle_doo();
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
            that there is more to read.

****************************************************************************************************
*/
osalStatus mcp3208_proc_resp(struct PinsBusDevice *device)
{
    PinsMcp3208Ext *ext;
    os_uchar *buf, current_ch;
    os_short *adc_value;

    buf = device->bus->buf;
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
   @return  None

****************************************************************************************************
*/
void mcp3208_set(struct PinsBusDevice *device, os_short addr, os_int value)
{
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



/*

rawMCP3008.c
Public Domain
2016-03-20

gcc -Wall -pthread -o rawMCP3008 rawMCP3008.c -lpigpio

This code shows how to bit bang SPI using DMA.

Using DMA to bit bang allows for two advantages

1) the time of the SPI transaction can be guaranteed to within a
   microsecond or so.

2) multiple devices of the same type can be read or written
  simultaneously.

This code shows how to read more than one MCP3008 at a time.

Each MCP3008 shares the SPI clock, MOSI, and slave select lines but has
a unique MISO line.

This code should also work with the MCP3004.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <pigpio.h>

// #define SPI_SS 25 // GPIO for slave select.
#define SPI_SS 8 // GPIO for slave select.

#define ADCS 1    // Number of connected MCP3008.

#define BITS 10            // Bits per reading.
// #define BITS 12            // Bits per reading.
#define BX 8               // Bit position of data bit B9.
#define B0 (BX + BITS - 1) // Bit position of data bit B0.

// #define MISO1 6   // ADC 1 MISO.
#define MISO1 9   // ADC 1 MISO.
#define MISO2 26  //     2
#define MISO3 13  //     3
#define MISO4 23  //     4
#define MISO5 24  //     5

#define BUFFER 250       // Generally make this buffer as large as possible.

#define REPEAT_MICROS 40 // Reading every x microseconds.

#define SAMPLES 2000000  // Number of samples to take,

// int MISO[ADCS]={MISO1, MISO2, MISO3, MISO4, MISO5};
int MISO[ADCS]={MISO1};

rawSPI_t rawSPI =
{
   .clk     =  11, // 5, // GPIO for SPI clock.
   .mosi    = 10, // 12, // GPIO for SPI MOSI.
   .ss_pol  =  1, // Slave select resting level.
   .ss_us   =  1, // Wait 1 micro after asserting slave select.
   .clk_pol =  0, // Clock resting level.
   .clk_pha =  0, // 0 sample on first edge, 1 sample on second edge.
   .clk_us  =  1, // 2 clocks needed per bit so 500 kbps.
};

/*
   This function extracts the MISO bits for each ADC and
   collates them into a reading per ADC.
*/

void getReading(
   int adcs,  // Number of attached ADCs.
   int *MISO, // The GPIO connected to the ADCs data out.
   int OOL,   // Address of first OOL for this reading.
   int bytes, // Bytes between readings.
   int bits,  // Bits per reading.
   char *buf)
{
   int i, a, p;
   uint32_t level;

   p = OOL;

   for (i=0; i<bits; i++)
   {
      level = rawWaveGetOut(p);

      for (a=0; a<adcs; a++)
      {
         putBitInBytes(i, buf+(bytes*a), level & (1<<MISO[a]));
      }

      p--;
   }
}


static int ulle_doo(void)
{
   int i, wid, offset;
   char buf[2];
   gpioPulse_t final[2];
   char rx[8];
   int sample;
   int val;
   int cb, botCB, topOOL, reading, now_reading;
   float cbs_per_reading;
   rawWaveInfo_t rwi;
   double start, end;
   int pause;

   /* if (argc > 1) pause = atoi(argv[1]); else */ pause =0;

//   if (gpioInitialise() < 0) return 1;

   // Need to set GPIO as outputs otherwise wave will have no effect.

   gpioSetMode(rawSPI.clk,  PI_OUTPUT);
   gpioSetMode(rawSPI.mosi, PI_OUTPUT);
   gpioSetMode(SPI_SS,      PI_OUTPUT);

   gpioWaveAddNew(); // Flush any old unused wave data.

   offset = 0;

   /*
   MCP3004/8 10-bit ADC 4/8 channels

   1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16 17
   SB SD D2 D1 D0 NA NA B9 B8 B7 B6 B5 B4 B3 B2 B1 B0

   SB       1
   SD       0=differential 1=single
   D2/D1/D0 0-7 channel
   */

   /*
      Now construct lots of bit banged SPI reads.  Each ADC reading
      will be stored separately.  We need to ensure that the
      buffer is big enough to cope with any reasonable rescehdule.

      In practice make the buffer as big as you can.
   */

   for (i=0; i<BUFFER; i++)
   {
      buf[0] = 0xC0; // Start bit, single ended, channel 0.

      rawWaveAddSPI(&rawSPI, offset, SPI_SS, buf, 2, BX, B0, B0);

      /*
         REPEAT_MICROS must be more than the time taken to
         transmit the SPI message.
      */

      offset += REPEAT_MICROS;
   }

   // Force the same delay after the last reading.

   final[0].gpioOn = 0;
   final[0].gpioOff = 0;
   final[0].usDelay = offset;

   final[1].gpioOn = 0; // Need a dummy to force the final delay.
   final[1].gpioOff = 0;
   final[1].usDelay = 0;

   gpioWaveAddGeneric(2, final);

   wid = gpioWaveCreate(); // Create the wave from added data.

   if (wid < 0)
   {
      fprintf(stderr, "Can't create wave, %d too many?\n", BUFFER);
      return 1;
   }

   /*
      The wave resources are now assigned,  Get the number
      of control blocks (CBs) so we can calculate which reading
      is current when the program is running.
   */

   rwi = rawWaveInfo(wid);

   printf("# cb %d-%d ool %d-%d del=%d ncb=%d nb=%d nt=%d\n",
      rwi.botCB, rwi.topCB, rwi.botOOL, rwi.topOOL, rwi.deleted,
      rwi.numCB,  rwi.numBOOL,  rwi.numTOOL);

   /*
      CBs are allocated from the bottom up.  As the wave is being
      transmitted the current CB will be between botCB and topCB
      inclusive.
   */

   botCB = rwi.botCB;

   /*
      Assume each reading uses the same number of CBs (which is
      true in this particular example).
   */

   cbs_per_reading = (float)rwi.numCB / (float)BUFFER;

   printf("# cbs=%d per read=%.1f base=%d\n",
      rwi.numCB, cbs_per_reading, botCB);

   /*
      OOL are allocated from the top down. There are BITS bits
      for each ADC reading and BUFFER ADC readings.  The readings
      will be stored in topOOL - 1 to topOOL - (BITS * BUFFER).
   */

   topOOL = rwi.topOOL;

   fprintf(stderr, "starting...\n");

   if (pause) time_sleep(pause); // Give time to start a monitor.

   gpioWaveTxSend(wid, PI_WAVE_MODE_REPEAT);

   reading = 0;

   sample = 0;

   start = time_time();

   while (sample<SAMPLES)
   {
      // Which reading is current?

      cb = rawWaveCB() - botCB;

      now_reading = (float) cb / cbs_per_reading;

      // Loop gettting the fresh readings.

      while (now_reading != reading)
      {
         /*
            Each reading uses BITS OOL.  The position of this readings
            OOL are calculated relative to the waves top OOL.
         */
         getReading(
            ADCS, MISO, topOOL - ((reading%BUFFER)*BITS) - 1, 2, BITS, rx);

         printf("%d", ++sample);

         for (i=0; i<ADCS; i++)
         {
            //  7  6  5  4  3  2  1  0 |  7  6  5  4  3  2  1  0
            // B9 B8 B7 B6 B5 B4 B3 B2 | B1 B0  X  X  X  X  X  X

            val = (rx[i*2]<<2) + (rx[(i*2)+1]>>6);
            printf(" %d", val);
         }

         printf("\n");

         if (++reading >= BUFFER) reading = 0;
      }
      usleep(1000);
   }

   end = time_time();

   printf("# %d samples in %.1f seconds (%.0f/s)\n",
      SAMPLES, end-start, (float)SAMPLES/(end-start));

   fprintf(stderr, "ending...\n");

   if (pause) time_sleep(pause);

  // gpioTerminate();

   return 0;
}


#endif
#endif

