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

  I2c device address 64 (0x40) is default for Adafruit PCA9685.

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
#if PINS_I2C
#if PINS_MAX_PCA9685_PWM

#define PCA9685_NRO_PWM_CHANNELS 16

#define PCA9685_MAX_REPLY_BYTES 2

typedef enum {
    PCA9695_NOT_INITIALIZED = 0,
    PCA9695_INIT_STARTING,
    PCA9695_INIT_MODE_QUARY,
    PCA9695_SET_PWM_FREQ,
    PCA9695_SET_PWM_FREQ2,
    PCA9695_INIT_FINISHED,

    PCA9695_RESET_I2C_BUS
}
pca9685InitStep;

typedef struct PinsPca9685Ext
{
    /* Current duty cycle values for each PWM channel.
     */
    os_short pwm_value[PCA9685_NRO_PWM_CHANNELS];

    /* Initialization sequence step and status.
     */
    pca9685InitStep initialization_step;
    osalStatus init_status;

    /* PWM pulse frequency, for example 60 (Hz). All PCA9685 pins share same frequency.
     */
    os_short pwm_frequency;

    /* Currently selected channel.
     */
    os_uchar current_ch;

    /* Saved mode byte for initialization
     */
    os_uchar mode_1;

    /* Byte reply 0 - 255 from device. os_short as type because -1 indicates not set.
     */
    os_short reply_byte[PCA9685_MAX_REPLY_BYTES];
}
PinsPca9685Ext;


/** Registers.
 */
#define PCA9685_MODE1 0x00			/* Mode register  1 */
#define PCA9685_MODE2 0x01			/* Mode register  2 */
#define PCA9685_PRE_SCALE 0xFE		/* prescaler for output frequency */
#define PCA9685_CLOCK_FREQ 25000000.0 /* 25MHz default osc clock */

/* #define SUBADR1 0x02	*/          /* I2C-bus subaddress 1 */
/* #define SUBADR2 0x03 */          /* I2C-bus subaddress 2 */
/* #define SUBADR3 0x04	*/          /* I2C-bus subaddress 3 */
/* #define ALLCALLADR 0x05 */       /* LED All Call I2C-bus address */

#define PCA9685_CH0 0x6             /* CH0 start register */
#define PCA9685_CH0_ON_L 0x6		/* CH0 output and brightness control byte 0 */
#define PCA9685_CH0_ON_H 0x7		/* CH0 output and brightness control byte 1 */
#define PCA9685_CH0_OFF_L 0x8		/* CH0 output and brightness control byte 2 */
#define PCA9685_CH0_OFF_H 0x9		/* CH0 output and brightness control byte 3 */
#define PCA9685_CH_MULTIPLYER 4     /*  For the other 15 channels */

#define PCA9685_ALL_LED_ON_L 0xFA   /* load all the LEDn_ON registers, byte 0 (turn 0-7 channels on) */
#define PCA9685_ALL_LED_ON_H 0xFB	/* load all the LEDn_ON registers, byte 1 (turn 8-15 channels on) */
#define PCA9685_ALL_LED_OFF_L 0xFC	/* load all the LEDn_OFF registers, byte 0 (turn 0-7 channels off) */
#define PCA9685_ALL_LED_OFF_H 0xFD	/* load all the LEDn_OFF registers, byte 1 (turn 8-15 channels off) */

/* Mode bits
 */
#define PCA9685_RESTART 0x80
#define PCA9685_SLEEP   0x10
#define PCA9685_ALLCALL 0x01
#define PCA9685_INVRT   0x10
#define PCA9685_OUTDRV  0x04

/* Global variables.
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

   @param   device Structure representing I2C device.
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
    ext->pwm_value[addr] = (os_short)value;

    value = pin_get_prm(pin, PIN_FREQENCY);
    if (value) {
#if OSAL_DEBUG
        if (ext->pwm_frequency && value != ext->pwm_frequency) {
            osal_debug_error("pca9685: PWM frequency must be same for all the pins");
        }
#endif
        ext->pwm_frequency = (os_short)value;
    }
}


/**
****************************************************************************************************

   @brief Set PWM frequency for the device.
   @anchor pca9685_initialization_sequence

   The pca9685_initialization_sequence() function sets PWM pulse frequency for all PWM channels
   40Hz to 1000Hz using internal 25MHz oscillator.

   Notice that same frequency needs to be used for all PWM outputs of the PCA9625 chip.

   @param   device Structure representing I2C device.
   @return  OSAL_COMPLETE if initialization sequence has been completed successfully.
            OSAL_SUCCESS if installation sequence step is prepared.
            Other values indicate an installation sequence error.

****************************************************************************************************
*/
static osalStatus pca9685_initialization_sequence(struct PinsBusDevice *device)
{
    struct PinsBus *bus;
    PinsPca9685Ext *ext;
    os_uchar prescale_val, *p;
    os_int frequency;
    osalStatus s = OSAL_SUCCESS;

    bus = device->bus;
    ext = (PinsPca9685Ext*)device->ext;
    p = bus->outbuf;

    os_timeslice();

    switch (ext->initialization_step)
    {
        case PCA9695_NOT_INITIALIZED:
            *(p++) = PCA9685_ALL_LED_ON_L;
            *(p++) = 0x00;
            *(p++) = PCA9685_ALL_LED_ON_H;
            *(p++) = 0x00;
            *(p++) = PCA9685_ALL_LED_OFF_L;
            *(p++) = 0x00;
            *(p++) = PCA9685_ALL_LED_OFF_H;
            *(p++) = 0x00;

            *(p++) = PCA9685_MODE2;
            *(p++) = PCA9685_OUTDRV;
            *(p++) = PCA9685_MODE1;
            *(p++) = PCA9685_ALLCALL;

            bus->spec.i2c.bus_operation = PINS_I2C_WRITE_BYTE_DATA;
            break;

        case PCA9695_INIT_STARTING:
            *(p++) = PCA9685_MODE1;
            *(p++) = PCA9685_MODE2;
            bus->spec.i2c.bus_operation = PINS_I2C_READ_BYTE_DATA;
            ext->reply_byte[0] = ext->reply_byte[1] = -1;
            break;

        case PCA9695_INIT_MODE_QUARY:
            if (ext->reply_byte[0] == -1) {
                /* ext->initialization_step = PCA9695_RESET_I2C_BUS; Reset doesn't do any good */
                ext->initialization_step = PCA9695_NOT_INITIALIZED;
                s = OSAL_STATUS_NOT_CONNECTED;
                goto getout;
            }
            ext->mode_1 = (os_uchar)ext->reply_byte[0] & ~PCA9685_RESTART;

            *(p++) = PCA9685_MODE1;
            *(p++) = PCA9685_RESTART | ext->mode_1;
            bus->spec.i2c.bus_operation = PINS_I2C_WRITE_BYTE_DATA;
            break;

        case PCA9695_SET_PWM_FREQ:
            frequency = ext->pwm_frequency;
            if (frequency <= 0) frequency = 60;
            prescale_val = (unsigned char)((PCA9685_CLOCK_FREQ / (4096 * frequency)) - 1);

            *(p++) = PCA9685_MODE1;
            *(p++) = PCA9685_SLEEP | ext->mode_1;

            *(p++) = PCA9685_PRE_SCALE;
            *(p++) = prescale_val;

            *(p++) = PCA9685_MODE1;
            *(p++) = ext->mode_1;
            break;

        case PCA9695_SET_PWM_FREQ2:
            *(p++) = PCA9685_RESTART;
            *(p++) = ext->mode_1;
            break;

        default:
        case PCA9695_INIT_FINISHED:
            s = OSAL_COMPLETED;
            break;

        case PCA9695_RESET_I2C_BUS:
            ext->initialization_step = PCA9695_NOT_INITIALIZED;
            s = OSAL_STATUS_NOT_CONNECTED;
            goto getout;
    }

    ext->initialization_step++;

getout:
    bus->outbuf_n = (os_short)(p - bus->outbuf);
    return s;
}


/**
****************************************************************************************************

   @brief Prepare request to send
   @anchor pca9685_gen_req

   The pca9685_gen_req() function prepares the next request to send to the device into buffer
   within the bus struture.

   @param   device Structure representing I2C device.
   @return  OSAL_COMPLETED indicates that this was last I2C transaction needed for this device
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

    if (ext->init_status != OSAL_COMPLETED)
    {
        ext->init_status = pca9685_initialization_sequence(device);
        if (ext->init_status != OSAL_COMPLETED) {
            return ext->init_status;
        }
    }

    current_ch = ext->current_ch;
    p = buf;
    ch_count = 0;
    check_count = PCA9685_NRO_PWM_CHANNELS;
    while (check_count-- && ch_count < max_ch_at_once) {
        off_value = ext->pwm_value[current_ch];
        if (off_value >= 0) {
            offs = (os_uchar)(PCA9685_CH_MULTIPLYER * current_ch);

            /*  0-4095 value to turn on the pulse */
            *(p++) = PCA9685_CH0_ON_L + offs;
            *(p++) = (os_uchar)on_value;
            *(p++) = PCA9685_CH0_ON_H + offs;
            *(p++) = (os_uchar)(on_value >> 8);

            /*  0-4095 value to turn off the pulse */
            *(p++) = PCA9685_CH0_OFF_L + offs;
            *(p++) = (os_uchar)off_value;
            *(p++) = PCA9685_CH0_OFF_H + offs;
            *(p++) = (os_uchar)(off_value >> 8);

            ++ch_count;
        }

        if (++current_ch >= PCA9685_NRO_PWM_CHANNELS) {
            s = OSAL_COMPLETED;
            current_ch = 0;
        }
    }

    ext->current_ch = current_ch;
    bus->outbuf_n = (os_short)(p - buf);
    bus->spec.i2c.bus_operation = PINS_I2C_WRITE_BYTE_DATA;
    return s;
}


/**
****************************************************************************************************

   @brief Process reply from I2C device
   @anchor pca9685_proc_resp

   The pca9685_proc_resp() function processed the received reply from buffer
   within the bus struture. It stores PWM value for the channel for the device

   Note: Sensibility checks for replay should be added, plus some kind of error counter would
   be appropriate to know if the design is failing.

   @param   device Structure representing I2C device.
   @return  OSAL_COMPLETED indicates that this was last I2C transaction needed for this device
            so that all data has been transferred from device. Value OSAL_SUCCESS to indicates
            that there is more to read. Other values indicate that I2C reply was not
            recieved or was errornous.

****************************************************************************************************
*/
osalStatus pca9685_proc_resp(struct PinsBusDevice *device)
{
    PinsBus *bus;
    PinsPca9685Ext *ext;
    os_short i;

    bus = device->bus;
    ext = (PinsPca9685Ext*)device->ext;

    /* Save byte reply */
    if (bus->inbuf_n <= PCA9685_MAX_REPLY_BYTES)
    {
        for (i = 0; i<bus->inbuf_n; i++) {
            ext->reply_byte[i] = bus->inbuf[i];
        }
    }

    return OSAL_COMPLETED;
}


/**
****************************************************************************************************

   @brief Set data to I2C device
   @anchor pca9685_set

   The pca9685_set() function is not needed for PWM, it is read only.

   @param   device Structure representing I2C device.
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
        return OSAL_STATUS_FAILED;
    }

    ext = (PinsPca9685Ext*)(device->ext);
    ext->pwm_value[addr] = (os_short)value;
    return ext->initialization_step == PCA9695_INIT_FINISHED
        ? OSAL_SUCCESS : OSAL_STATUS_NOT_CONNECTED;
}


/**
****************************************************************************************************

   @brief Get I2C device data
   @anchor pca9685_get

   The pca9685_get() function reads ASC channel value received from the device.

   @param   device Structure representing I2C device.
   @param   addr PWM channel 0 ... 7.
   @return  value PWM value received 0 ... 4095. -1 if none read.

****************************************************************************************************
*/
os_int pca9685_get(struct PinsBusDevice *device, os_short addr)
{
    PinsPca9685Ext *ext;
    ext = (PinsPca9685Ext*)(device->ext);
    osal_debug_assert(ext != OS_NULL);

    if (addr < 0 || addr >= PCA9685_NRO_PWM_CHANNELS ||
        ext->initialization_step != PCA9695_INIT_FINISHED)
    {
        return -1;
    }

    return ext->pwm_value[addr];
}

#endif
#endif

