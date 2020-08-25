/**

  @file    extensions/bus_drivers/common/pins_pwm_pca9685.c
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
#ifndef PINS_MAX_PCA9685_PWM
#define PINS_MAX_PCA9685_PWM 1
#endif

#include "pinsx.h"
#if PINS_SPI
#if PINS_MAX_PCA9685_PWM

#define PCA9685_NRO_PWM_CHANNELS 16

typedef struct PinsPca9685Ext
{
    os_short pwm_value[PCA9685_NRO_PWM_CHANNELS];
    os_uchar current_ch;
    os_short pwm_frequency;
    os_short pwm_freq_count;
    os_boolean connected;
}
PinsPca9685Ext;


/** I2c device address, 0x40 is default for Adafruit PCA9685.
 */
/*static int
    oepwm_i2c_address = 0x40;
*/
#define OEPI_MODE1 0x00			//Mode  register  1
#define OEPI_MODE2 0x01			//Mode  register  2
#define PRE_SCALE 0xFE		//prescaler for output frequency
#define CLOCK_FREQ 25000000.0 //25MHz default osc clock

/* #define SUBADR1 0x02		//I2C-bus subaddress 1
#define SUBADR2 0x03		//I2C-bus subaddress 2
#define SUBADR3 0x04		//I2C-bus subaddress 3
#define ALLCALLADR 0x05     //LED All Call I2C-bus address
*/
#define OEPI_CH0 0x6			//OEPI_CH0 start register
#define OEPI_CH0_ON_L 0x6		//OEPI_CH0 output and brightness control byte 0
#define OEPI_CH0_ON_H 0x7		//OEPI_CH0 output and brightness control byte 1
#define OEPI_CH0_OFF_L 0x8		//OEPI_CH0 output and brightness control byte 2
#define OEPI_CH0_OFF_H 0x9		//OEPI_CH0 output and brightness control byte 3
#define OEPI_CH_MULTIPLYER 4	// For the other 15 channels

/* #define ALLLED_ON_L 0xFA    //load all the LEDn_ON registers, byte 0 (turn 0-7 channels on)
#define ALLLED_ON_H 0xFB	//load all the LEDn_ON registers, byte 1 (turn 8-15 channels on)
#define ALLLED_OFF_L 0xFC	//load all the LEDn_OFF registers, byte 0 (turn 0-7 channels off)
#define ALLLED_OFF_H 0xFD	//load all the LEDn_OFF registers, byte 1 (turn 8-15 channels off)
*/

static PinsPca9685Ext pca9685_ext[PINS_MAX_PCA9685_PWM];
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
   @anchor pca9685_initialize_device

   The pca9685_initialize_device() function initializes a bus device structure for a specific
   PCA9685 chip.

   @param   device Structure representing SPI device.
   @return  None.

****************************************************************************************************
*/
void pca9685_initialize_device(struct PinsBusDevice *device)
{
    PinsBusDeviceParams prm;
    os_short *pwm_value, i;

    if (pca9685_nro_chips >= PINS_MAX_PCA9685_PWM) {
        osal_debug_error("Number of PCA9685 chip exceeded in JSON, increase PINS_MAX_PCA9685_PWM");
        return;
    }

    device->ext = &pca9685_ext[pca9685_nro_chips++];
    pwm_value = ((PinsPca9685Ext*)(device->ext))->pwm_value;
    for (i = 0; i < PCA9685_NRO_PWM_CHANNELS; i++) {
        pwm_value[i] = -1;
    }

// First test without error handling
((PinsPca9685Ext*)(device->ext))->connected = OS_TRUE;

    /* Call platform specific device initialization.
     */
    os_memclear(&prm, sizeof(prm));
    pins_init_device(device, &prm);
}


/**
****************************************************************************************************

   @brief Initialize "pin" of bus device.
   @anchor pca9685_initialize_pin

   The pca9685_initialize_pin() function initializes a bus device's pin. In practice this
   function may set some data in device strucure.

   @param   pin Structure representing bus device "pin".
   @return  None.

****************************************************************************************************
*/
void pca9685_initialize_pin(const struct Pin *pin)
{
    PinsBusDevice *device;
    PinsPca9685Ext*ext;
    os_short addr;
    os_int value;

    device = pin->bus_device;
    osal_debug_assert(device != OS_NULL);

    addr = pin->addr;
    osal_debug_assert(addr >= 0 && addr < PCA9685_NRO_PWM_CHANNELS);

    ext = (PinsPca9685Ext*)(device->ext);
    osal_debug_assert(ext != OS_NULL);

    value = pin_get_prm(pin, PIN_INIT);
    ext->pwm_value[addr] = value;

    value = pin_get_prm(pin, PIN_FREQENCY);
    if (value) {
#if OSAL_DEBUG
        if (ext->pwm_frequency && value != ext->pwm_frequency) {
            osal_debug_error("pca9685: Frequency must be same for all pins");
        }
#endif
        ext->pwm_frequency = value;
    }
}


/**
****************************************************************************************************

   @brief Set PWM frequency for the device.
   @anchor pca9685_set_pwm_freq

   The pca9685_set_pwm_freq() function sets PWM pulse frequency for all PWM channels
   40Hz to 1000Hz using internal 25MHz oscillator.

   Notice that same frequency needs to be used for all PWM outputs of the PCA9625 chip.

   /............ @param   pin Structure representing I2C bus.
   ........ @param   frequency PWM frequency, typically 60 Hz for servos. Higher for LEDs.
   @return  None.

****************************************************************************************************
*/
static void pca9685_set_pwm_freq(struct PinsBusDevice *device)
{
    struct PinsBus *bus;
    PinsPca9685Ext *ext;
    os_uchar prescale_val, *p;
    os_int frequency;

    bus = device->bus;
    ext = (PinsPca9685Ext*)device->ext;
    p = bus->outbuf;

    switch (ext->pwm_freq_count)
    {
        case 0:
            *(p++) = OEPI_MODE1;
            *(p++) = 0x00;
            *(p++) = OEPI_MODE2;
            *(p++) = 0x04;
            break;

        case 1:
            os_timeslice();

            frequency = ext->pwm_frequency;
            if (frequency <= 0) frequency = 60;
            prescale_val = (unsigned char)((CLOCK_FREQ / 4096 / frequency) - 1);

            /* sleep
             */
            *(p++) = OEPI_MODE1;
            *(p++) = 0x10;

            /* multiplyer for PWM frequency.
             */
            *(p++) = PRE_SCALE;
            *(p++) = prescale_val;
            break;

        case 2:
            os_timeslice();
            /* restart.
             */
            *(p++) = OEPI_MODE1;
            *(p++) = 0x80;
            break;

        case 3:
            os_timeslice();
            /* totem pole (default)
             */
            *(p++) = OEPI_MODE2;
            *(p++) = 0x04;
            break;
    }

    ext->pwm_freq_count++;
    bus->outbuf_n = (os_short)(p - bus->outbuf);
    bus->inbuf_n = 0;
}


/**
****************************************************************************************************

   @brief Prepare request to send
   @anchor pca9685_gen_req

   The pca9685_gen_req() function prepares the next request to send to the device into buffer
   within the bus struture.

   @param   device Structure representing SPI device.
   @return  OSAL_COMPLETED indicates that this was last SPI transaction needed for this device
            so that all data has been transferred to device. Value OSAL_SUCCESS to indicates
            that there is more to write (or that checking is done when processing reply).

****************************************************************************************************
*/
osalStatus pca9685_gen_req(struct PinsBusDevice *device)
{
    PinsBus *bus;
    PinsPca9685Ext *ext;
    os_uchar *buf, *p, current_ch, offs;
    os_short ch_count, check_count, on_value = 0, off_value;
    const os_int max_ch_at_once = 1;
    osalStatus s = OSAL_SUCCESS;

    bus = device->bus;
    osal_debug_assert(bus != OS_NULL);

    buf = bus->outbuf;
    ext = (PinsPca9685Ext*)device->ext;

    if (ext->pwm_freq_count <= 2)
    {
        pca9685_set_pwm_freq(device);
        // ext->pwm_freq_count++;
        return s;
    }

    current_ch = ext->current_ch;
    p = buf;
    ch_count = 0;
    check_count = PCA9685_NRO_PWM_CHANNELS;
    while (check_count--) {
        off_value = ext->pwm_value[current_ch];
        if (off_value >= 0) {
            offs = (os_uchar)(OEPI_CH_MULTIPLYER * current_ch);

            /*  0-4095 value to turn on the pulse */
            *(p++) = OEPI_CH0_ON_L + offs;
            *(p++) = (os_uchar)on_value;
            *(p++) = OEPI_CH0_ON_H + offs;
            *(p++) = (os_uchar)(on_value >> 8);

            /*  0-4095 value to turn off the pulse */
            *(p++) = OEPI_CH0_OFF_L + offs;
            *(p++) = (os_uchar)off_value;
            *(p++) = OEPI_CH0_OFF_H + offs;
            *(p++) = (os_uchar)(off_value >> 8);

            if (++ch_count >= max_ch_at_once) break;
        }

        if (++current_ch >= PCA9685_NRO_PWM_CHANNELS) {
            s = OSAL_COMPLETED;
            current_ch = 0;
        }
    }

    ext->current_ch = current_ch;
    bus->outbuf_n = (os_short)(p - buf);
    bus->inbuf_n = 0;
    return s;
}


/**
****************************************************************************************************

   @brief Process reply from SPI device
   @anchor pca9685_proc_resp

   The pca9685_proc_resp() function processed the received reply from buffer
   within the bus struture. It stores PWM value for the channel for the device

   Note: Sensibility checks for replay should be added, plus some kind of error counter would
   be appropriate to know if the design is failing.

   @param   device Structure representing SPI device.
   @return  OSAL_COMPLETED indicates that this was last SPI transaction needed for this device
            so that all data has been transferred from device. Value OSAL_SUCCESS to indicates
            that there is more to read. Other values indicate that I2C reply was not
            recieved or was errornous.

****************************************************************************************************
*/
osalStatus pca9685_proc_resp(struct PinsBusDevice *device)
{
    PinsPca9685Ext *ext;
    os_uchar *buf, current_ch;
    os_short *pwm_value;

    buf = device->bus->inbuf;
    ext = (PinsPca9685Ext*)device->ext;
    current_ch = ext->current_ch;
    pwm_value = ext->pwm_value;
    pwm_value[current_ch] = (os_short)(((os_ushort)(buf[1] & 0x0F) << 8) | (os_ushort)buf[2]);

    if (++current_ch < PCA9685_NRO_PWM_CHANNELS) {
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

   The pca9685_set() function is not needed for PWM, it is read only.

   @param   device Structure representing SPI device.
   @param   addr PWM channel 0 ... 7.
   @param   value Value to set, ignored.
   @return  OSAL_STATUS if successfull. Other values indicate a hardware error, specifically
            OSAL_STATUS_NOT_CONNECTED if I2C device is not connected.

****************************************************************************************************
*/
osalStatus pca9685_set(struct PinsBusDevice *device, os_short addr, os_int value)
{
    PinsPca9685Ext *ext;
    ext = (PinsPca9685Ext*)(device->ext);
    osal_debug_assert(ext != OS_NULL);

    if (addr < 0 || addr >= PCA9685_NRO_PWM_CHANNELS) {
        return -1;
    }

    ext = (PinsPca9685Ext*)(device->ext);
    ext->pwm_value[addr] = value;
    return ext->connected ? OSAL_SUCCESS : OSAL_STATUS_NOT_CONNECTED;
}


/**
****************************************************************************************************

   @brief Get SPI device data
   @anchor pca9685_get

   The pca9685_get() function reads ASC channel value received from the device.

   @param   device Structure representing SPI device.
   @param   addr PWM channel 0 ... 7.
   @return  value PWM value received 0 ... 4095. -1 if none read.

****************************************************************************************************
*/
os_int pca9685_get(struct PinsBusDevice *device, os_short addr)
{
    PinsPca9685Ext *ext;
    ext = (PinsPca9685Ext*)(device->ext);
    osal_debug_assert(ext != OS_NULL);

    if (addr < 0 || addr >= PCA9685_NRO_PWM_CHANNELS || !ext->connected)
    {
        return -1;
    }

    return ext->pwm_value[addr];
}

#endif
#endif

