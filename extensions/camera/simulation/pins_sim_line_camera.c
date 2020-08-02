/**

  @file    extensions/camera/simulation/pins_sim_line_camera.c
  @brief   Camera hardware API.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    21.4.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#define PINS_OS_INT_HANDLER_HDRS 1
#include "pinsx.h"
#if PINS_CAMERA == PINS_TCD1304_CAMERA

#define TDC1304_TIMING_CLOCK_HZ 200000.0
// #define TDC1304_DATA_SZ (3694/2)
#define TDC1304_DATA_SZ 3694

// #ifndef TDC1304_MAX_CAMERAS
// #define TDC1304_MAX_CAMERAS 1
// #endif

#define TCD1304_MAX_PIN_PRM 14

#define PINS_TCD1304_BUF_SZ (sizeof(iocBrickHdr) + TDC1304_DATA_SZ)

/* Extensions to PinsCamera structure
 */
typedef struct PinsCameraExt
{
    os_int prm[PINS_NRO_CAMERA_PARAMS];
    pinsCamera *c;
    volatile os_short pos;
    volatile os_short processed_pos;

    os_uchar buf[PINS_TCD1304_BUF_SZ];

    volatile os_boolean start_new_frame;
    volatile os_boolean frame_ready;

    os_int igc_on_pulse_setting;
    os_int igc_off_pulse_setting;

#if PINS_SIMULATED_INTERRUPTS
    PinInterruptConf loopback_int_conf;
#endif

    /* Pin parameters.
     */
    os_ushort sh_prm_count;
    os_ushort igc_prm_count;
    os_ushort igc_loopback_prm_count;
    os_ushort sh_pin_prm[TCD1304_MAX_PIN_PRM];
    os_ushort igc_pin_prm[TCD1304_MAX_PIN_PRM];
    os_ushort igc_loopback_pin_prm[TCD1304_MAX_PIN_PRM];

    /** Signal input and timing output pins.
     */
    Pin in_pin, igc_pin, sh_pin, igc_loopback_pin;
}
PinsCameraExt;

static PinsCameraExt camext;

/* Forward referred static functions.
 */
TIMER_INTERRUPT_HANDLER_PROTO(tcd1304_cam_1_on_timer);

static void tcd1304_cam_task(
    void *prm,
    osalEvent done);

static void tcd1304_setup_camera_io_pins(
    pinsCamera *c);


/**
****************************************************************************************************

  @brief Initialize global variables for cameras.
  @anchor tcd1304_cam_initialize

  The tcd1304_cam_initialize() clear global variables for the camera(s). This is necessary to
  when running in microcontroller which doesn't clear memory during soft reboot.

  @return  None

****************************************************************************************************
*/
static void tcd1304_cam_initialize(
    void)
{
}


/**
****************************************************************************************************

  @brief Get information about available cameras.
  @anchor esp32_enumerate_cameras

  The esp32_enumerate_cameras() function returns number of cameras currently available
  and optionally camera information. 

  @param   camera_info Where to store pointer to camera info. The argument can be OS_NULL if 
           only number of available cameras is needed. The enumerate_cameras function can 
           also set the camera_info pointer to OS_NULL if it doesn't provide any camera 
           information.
           If information structure is returned, it must be released by calling
           pins_release_camera_info() function.
           
  @return  Number of available cameras

****************************************************************************************************
*/
static os_int tcd1304_cam_enumerate_cameras(
    pinsCameraInfo **camera_info)
{
    if (camera_info) *camera_info = OS_NULL;
    return 1;
}


/**
****************************************************************************************************

  @brief Open the camera, set it up.
  @anchor tcd1304_cam_open

  The tcd1304_cam_open() sets ip camera for use. It creates threads, events, etc, nedessary
  for the camera. This function is called from  application trough camera interface
  pins_tcd1304_camera_iface.open().

  @param   c Pointer to camera structure.
  @return  OSAL_SUCCESS if all is fine. Other return values indicate an error.

****************************************************************************************************
*/
static osalStatus tcd1304_cam_open(
    pinsCamera *c,
    const pinsCameraParams *prm)
{
    osalThreadOptParams opt;

    os_memclear(c, sizeof(pinsCamera));
    c->camera_pin = prm->camera_pin;
    c->timer_pin = prm->timer_pin;
    c->callback_func = prm->callback_func;
    c->callback_context = prm->callback_context;

    camext.prm[PINS_CAM_INTEGRATION_US] = 2000;
    c->iface = &pins_tcd1304_camera_iface;
    c->ext = &camext;

    /* We could support two TDC1304 cameras later on, we should check which static camera
       structure is free, etc. Now one only.
     */
    /* Make sure that camera state structure is free (camera not already open)
       amd set up camera state.
     */
    os_memclear(&camext, sizeof(camext));
    camext.c = c;
    c->camera_nr = 0;

    /* Create event to trigger the thread.
     */
    c->camera_event = osal_event_create();

    /* Create thread that transfers camera frames.
     */
    os_memclear(&opt, sizeof(opt));
    opt.priority = OSAL_THREAD_PRIORITY_TIME_CRITICAL;
    opt.thread_name = "tcd1304";
    opt.pin_to_core = OS_TRUE;
    opt.pin_to_core_nr = 0;
    c->camera_thread = osal_thread_create(tcd1304_cam_task, c, &opt, OSAL_THREAD_ATTACHED);

    return OSAL_SUCCESS;
}


/**
****************************************************************************************************

  @brief Close the camera (release resources).
  @anchor tcd1304_cam_close

  The tcd1304_cam_close() stops the video and releases any resources reserved for the camera.
  This function is called from  application trough camera interface pins_tcd1304_camera_iface.close().

  @param   c Pointer to camera structure.
  @return  None

****************************************************************************************************
*/
static void tcd1304_cam_close(
    pinsCamera *c)
{
    if (c->camera_thread)
    {
        c->stop_thread = OS_TRUE;
        osal_event_set(c->camera_event);
        osal_thread_join(c->camera_thread);
        c->stop_thread = OS_FALSE;
        c->camera_thread = OS_NULL;
    }

    if (c->camera_event)
    {
        osal_event_delete(c->camera_event);
        c->camera_event = OS_NULL;
    }
}


/**
****************************************************************************************************

  @brief Start vido stream.
  @anchor tcd1304_cam_start

  The tcd1304_cam_start() starts the video. This function is called from  application trough
  camera interface pins_tcd1304_camera_iface.start().

  @param   c Pointer to camera structure.
  @return  None

****************************************************************************************************
*/
static void tcd1304_cam_start(
    pinsCamera *c)
{
    pinTimerParams prm;

    tcd1304_setup_camera_io_pins(c);

    os_memclear(&prm, sizeof(prm));
    prm.int_handler_func = tcd1304_cam_1_on_timer;

    pin_timer_attach_interrupt(c->timer_pin, &prm);

    camext.pos = 0;
    camext.processed_pos = 0;
    camext.start_new_frame = OS_FALSE;
    camext.frame_ready = OS_FALSE;
    pin_ll_set(&camext.igc_pin, camext.igc_on_pulse_setting);
}


/**
****************************************************************************************************

  @brief Stop vido stream.
  @anchor tcd1304_cam_stop

  The tcd1304_cam_stop() stops the video. This function is called from  application trough
  camera interface pins_tcd1304_camera_iface.stop().

  @param   c Pointer to camera structure.
  @return  None

****************************************************************************************************
*/
static void tcd1304_cam_stop(
    pinsCamera *c)
{
}


/**
****************************************************************************************************

  @brief Set value of camera parameter.
  @anchor tcd1304_cam_set_parameter

  The tcd1304_cam_set_parameter() sets value of a camera parameter. This function is called from
  application trough camera interface pins_tcd1304_camera_iface.set_parameter().

  @param   c Pointer to camera structure.
  @param   ix Parameter index, see enumeration pinsCameraParamIx.
  @param   x Parameter value.
  @return  None

****************************************************************************************************
*/
static void tcd1304_cam_set_parameter(
    pinsCamera *c,
    pinsCameraParamIx ix,
    os_long x)
{
    if (ix < 0 || ix >= PINS_NRO_CAMERA_PARAMS || x < 0) return;
    if ((os_int)camext.prm[ix] == x) return;
    camext.prm[ix] = (os_int)x;
}


/**
****************************************************************************************************

  @brief Get value of camera parameter.
  @anchor tcd1304_cam_get_parameter

  The tcd1304_cam_get_parameter() gets value of a camera parameter. This function is called from
  application trough camera interface pins_tcd1304_camera_iface.get_parameter().

  @param   c Pointer to camera structure.
  @param   ix Parameter index, see enumeration pinsCameraParamIx.
  @return  Parameter value.

****************************************************************************************************
*/
static os_long tcd1304_cam_get_parameter(
    pinsCamera *c,
    pinsCameraParamIx ix)
{
    if (ix < 0 || ix >= PINS_NRO_CAMERA_PARAMS) return -1;
    return camext.prm[ix];
}


/**
****************************************************************************************************

  @brief Set up pinsPhoto structure.
  @anchor tcd1304_finalize_camera_photo

  The tcd1304_finalize_camera_photo() sets up pinsPhoto structure "photo" to contain the grabbed
  image. Camera API passed photos to application callback with pointer to this photo structure.

  @param   c Pointer to camera structure.
  @param   photo Pointer to photo structure to set up.
  @return  None.

****************************************************************************************************
*/
static void tcd1304_finalize_camera_photo(
    pinsCamera *c,
    pinsPhoto *photo)
{
    iocBrickHdr *hdr;
    os_uchar *buf;

    os_memclear(photo, sizeof(pinsPhoto));
    buf = camext.buf;
    os_memclear(buf, sizeof(iocBrickHdr));

    photo->iface = c->iface;
    photo->camera = c;
    photo->data = buf + sizeof(iocBrickHdr);
    photo->data_sz = TDC1304_DATA_SZ;
    hdr = (iocBrickHdr*)buf;
    hdr->alloc_sz[0] = (os_uchar)PINS_TCD1304_BUF_SZ;
    hdr->alloc_sz[1] = (os_uchar)(PINS_TCD1304_BUF_SZ >> 8);
    photo->hdr = hdr;

    photo->byte_w = TDC1304_DATA_SZ;
    photo->w = TDC1304_DATA_SZ;
    photo->h = 1;
    photo->format = hdr->format;
}


/**
****************************************************************************************************

  @brief Thread to process camera data.
  @anchor tcd1304_cam_task

  The tcd1304_cam_task() is high priority thread to process data from camera.

  @param   prm Pointer to pinsCamera structure.
  @param   done Even to be set to allow thread which created this one to proceed.
  @return  None.

****************************************************************************************************
*/
static void tcd1304_cam_task(
    void *prm,
    osalEvent done)
{
    pinsPhoto photo;
    pinsCamera *c;
    os_int x;
    os_short pos, processed_pos, max_pos;

    c = (pinsCamera*)prm;

    osal_event_set(done);

int dummy = 0, xsum = 0, xn = 0;

    while (!c->stop_thread && osal_go())
    {
        if (osal_event_wait(c->camera_event, 2017) != OSAL_STATUS_TIMEOUT)
        {
            if (!camext.frame_ready)
            {
                pos = camext.pos;
                processed_pos = camext.processed_pos;

                if (processed_pos < TDC1304_DATA_SZ)
                {
                    x = pin_ll_get(&camext.in_pin);
                    // x = local_adc1_read_test(cam_state[c->camera_nr].in_pin.addr);

                    if (processed_pos == 0) {
                        pin_ll_set(&camext.igc_pin, camext.igc_off_pulse_setting);
                    }

                    max_pos = pos;
                    if (max_pos > TDC1304_DATA_SZ) {
                        max_pos = TDC1304_DATA_SZ;
                    }

                    x >>= 4;
                    while (processed_pos < max_pos) {
                        camext.buf[sizeof(iocBrickHdr) + processed_pos++] = x;
                    }
                    xsum += x;
                    xn ++;
                    camext.processed_pos = processed_pos;
                }

                if (pos > TDC1304_DATA_SZ + 30) // + 30 SLACK
                {
                    tcd1304_finalize_camera_photo(c, &photo);
                    c->callback_func(&photo, c->callback_context);

                    camext.frame_ready = OS_TRUE;
                    camext.processed_pos = 0;
                    pin_ll_set(&camext.igc_pin, camext.igc_on_pulse_setting);

    if (++dummy > 100 && xn) {
//        osal_debug_error_int("HERE average ",  xsum / xn);
//        osal_debug_error_int("HERE n ",  xn);
        dummy = 0;
    }
    xsum = xn = 0;

                }
            }
        }
    }
}


/**
****************************************************************************************************

  @brief IGC pin interrupt handler, rising edge.
  @anchor tcd1304_cam_1_igc_end

  The tcd1304_cam_1_igc_end() function is called shen IGC loopback input changes from 0 to 1,
  ending the IGC pulse (0 = active). This triggers recording of the new data frame.

****************************************************************************************************
*/
BEGIN_PIN_INTERRUPT_HANDLER(tcd1304_cam_1_igc_end)
    camext.start_new_frame = OS_TRUE;
END_PIN_INTERRUPT_HANDLER(tcd1304_cam_1_igc_end)


/**
****************************************************************************************************

  @brief Timer interrupt handler.
  @anchor tcd1304_cam_1_on_timer

  The tcd1304_cam_1_on_timer() function is called at timer interrupt, typically something like
  100kHz or 200kHz frequency. The function reads signal value from ADC number and/or triggers
  camera task.

****************************************************************************************************
*/
BEGIN_TIMER_INTERRUPT_HANDLER(tcd1304_cam_1_on_timer)
    if (camext.start_new_frame)
    {
        camext.pos = 0;
        camext.processed_pos = 0;
        camext.start_new_frame = OS_FALSE;
        camext.frame_ready = OS_FALSE;
    }
    else  {
        camext.pos++;
    }

    osal_event_set(camext.c->camera_event);

#if PINS_SIMULATED_INTERRUPTS
    if (camext.pos == TDC1304_DATA_SZ + 50)
    {
        tcd1304_cam_1_igc_end();
    }
#endif

END_TIMER_INTERRUPT_HANDLER(tcd1304_cam_1_on_timer)


/**
****************************************************************************************************

  @brief Append parameter to IO pin's parameter array (internal).
  @anchor tcd1304_append_pin_parameter

  The tcd1304_append_pin_parameter() is helper function for tcd1304_setup_camera_io_pins() to
  append a parameter to parameter array for an IO pin.

  @param   pin_prm Pointer to the parameter array for an IO pin.
  @param   pin_prm_count Pointer to number of parameters in parameter array, incremented by
           the function.
  @param   Parameter index, see enumeration pinPrm.
  @param   value Value for parameter to add.
  @return  None.

****************************************************************************************************
*/
static void tcd1304_append_pin_parameter(
    os_ushort *pin_prm,
    os_ushort *pin_prm_count,
    pinPrm prm,
    os_int value)
{
    os_ushort i;

    i = *pin_prm_count;
    if (i >= TCD1304_MAX_PIN_PRM - 1)
    {
        osal_debug_error("tcd1394: too many pin parameters");
        return;
    }

    pin_prm[i++] = prm;
    pin_prm[i++] = value;
    *pin_prm_count = i;
}


/**
****************************************************************************************************

  @brief Configure IO pins needed for the CCD (internal).
  @anchor tcd1304_setup_camera_io_pins

  The tcd1304_setup_camera_io_pins() function configures PWMs to generate clock M, SH and IGC
  signals for CCD. Then it configures input for looping back IGC to generate "start" interrupt
  and sets up timer to generate interrupt at sampling frequency.

  Typically pin numbers and timing parameters are set in JSON and stored within the structure c.

  @param   c Pointer to camera structure.
  @return  None.

****************************************************************************************************
*/
static void tcd1304_setup_camera_io_pins(
    pinsCamera *c)
{
    pinInterruptParams iprm;

    os_double
        sh_period_us,
        pulse_us,
        pulse_setting;

    os_int
        timer_nr,
        bits,
        max_pulse,
        clocks_per_sh,
        sh_frequency_hz,
        sh_pulse_setting,
        igc_pulse_setting,
        sh_delay_setting;

    /* Camera analog video signal input.
     */
    camext.in_pin.type = PIN_ANALOG_INPUT;
    camext.in_pin.addr = pin_get_prm(c->camera_pin, PIN_A);
    pin_ll_setup(&camext.in_pin, PINS_DEFAULT);

    /* Calculate timing
     */
    clocks_per_sh = os_round_int(0.000001 * camext.prm[PINS_CAM_INTEGRATION_US] * TDC1304_TIMING_CLOCK_HZ);
    if (clocks_per_sh < 10) clocks_per_sh = 10;
    sh_frequency_hz = os_round_int(TDC1304_TIMING_CLOCK_HZ / clocks_per_sh);
    if (sh_frequency_hz < 10) sh_frequency_hz = 10;
    sh_period_us = 1000000.0 / sh_frequency_hz;

    /* Determine bits to have approximately 0.1 us resolution.
     */
    bits = 16;
    while ( sh_period_us / (1ULL << bits) < 0.1)
        bits --;
    max_pulse = 1 << bits;

    /* SH pulse
     */
    pulse_us = 1.0;
    pulse_setting = max_pulse * pulse_us / sh_period_us;
    sh_pulse_setting = (os_int)(pulse_setting + 0.9999);
    if (sh_pulse_setting < 1) sh_pulse_setting = 1;

    /* IGC pulse
     */
    pulse_us = 5.0;
    pulse_setting = max_pulse * pulse_us / sh_period_us;
    igc_pulse_setting = (os_int)(pulse_setting + 0.9999);
    if (igc_pulse_setting < 1) igc_pulse_setting = 1;

    /* SH delay
     */
    pulse_us = 0.5;
    pulse_setting = max_pulse * pulse_us / sh_period_us;
    sh_delay_setting = os_round_int(pulse_setting);
    if (sh_delay_setting < 1) sh_delay_setting = 1;

    camext.igc_on_pulse_setting = max_pulse - igc_pulse_setting;
    camext.igc_off_pulse_setting = max_pulse;

    timer_nr = pin_get_prm(c->camera_pin, PIN_TIMER_SELECT);

    /* Integration time (electronic shutter) signal SH.
     */
    camext.sh_prm_count = 0;
    tcd1304_append_pin_parameter(camext.sh_pin_prm, &camext.sh_prm_count, PIN_RV, PIN_RV);
    tcd1304_append_pin_parameter(camext.sh_pin_prm, &camext.sh_prm_count, PIN_TIMER_SELECT, timer_nr);
    tcd1304_append_pin_parameter(camext.sh_pin_prm, &camext.sh_prm_count, PIN_FREQENCY, sh_frequency_hz);
    tcd1304_append_pin_parameter(camext.sh_pin_prm, &camext.sh_prm_count, PIN_RESOLUTION, bits);
    tcd1304_append_pin_parameter(camext.sh_pin_prm, &camext.sh_prm_count, PIN_INIT, sh_pulse_setting);
    tcd1304_append_pin_parameter(camext.sh_pin_prm, &camext.sh_prm_count, PIN_HPOINT, sh_delay_setting);

    camext.sh_pin.type = PIN_PWM;
    camext.sh_pin.bank = pin_get_prm(c->camera_pin, PIN_B_BANK); /* PWM channel */
    camext.sh_pin.addr = pin_get_prm(c->camera_pin, PIN_B);
    camext.sh_pin.prm = camext.sh_pin_prm;
    camext.sh_pin.prm_n = (os_char)camext.sh_prm_count;
    pin_ll_setup(&camext.sh_pin, PINS_DEFAULT);

    /* IGC parameters
     */
    camext.igc_prm_count = 0;
    tcd1304_append_pin_parameter(camext.igc_pin_prm, &camext.igc_prm_count, PIN_RV, PIN_RV);
    tcd1304_append_pin_parameter(camext.igc_pin_prm, &camext.igc_prm_count, PIN_TIMER_SELECT, timer_nr);
    tcd1304_append_pin_parameter(camext.igc_pin_prm, &camext.igc_prm_count, PIN_FREQENCY, sh_frequency_hz);
    tcd1304_append_pin_parameter(camext.igc_pin_prm, &camext.igc_prm_count, PIN_RESOLUTION, bits);
    tcd1304_append_pin_parameter(camext.igc_pin_prm, &camext.igc_prm_count, PIN_INIT, camext.igc_on_pulse_setting);
    tcd1304_append_pin_parameter(camext.igc_pin_prm, &camext.igc_prm_count, PIN_HPOINT, igc_pulse_setting);

    /* Integration clear (new photo) signal IGC.
     */
    camext.igc_pin.type = PIN_PWM;
    camext.igc_pin.bank = pin_get_prm(c->camera_pin, PIN_C_BANK); /* PWM channel */
    camext.igc_pin.addr = pin_get_prm(c->camera_pin, PIN_C);
    camext.igc_pin.prm = camext.igc_pin_prm;
    camext.igc_pin.prm_n = (os_char)camext.igc_prm_count;
    pin_ll_setup(&camext.igc_pin, PINS_DEFAULT);

    /* IGC loop back parameters
     */
    camext.igc_loopback_prm_count = 0;
    tcd1304_append_pin_parameter(camext.igc_loopback_pin_prm, &camext.igc_loopback_prm_count, PIN_RV, PIN_RV);
    tcd1304_append_pin_parameter(camext.igc_loopback_pin_prm, &camext.igc_loopback_prm_count, PIN_INTERRUPT_ENABLED, 1);

    /* Integration clear (new photo) signal IGC.
     */
    camext.igc_loopback_pin.type = PIN_INPUT;
    camext.igc_loopback_pin.addr = pin_get_prm(c->camera_pin, PIN_D);
    camext.igc_loopback_pin.prm = camext.igc_loopback_pin_prm;
    camext.igc_loopback_pin.prm_n = (os_char)camext.igc_loopback_prm_count;
#if PINS_SIMULATED_INTERRUPTS
    camext.igc_loopback_pin.int_conf = &camext.loopback_int_conf;
#endif
    pin_ll_setup(&camext.igc_loopback_pin, PINS_DEFAULT);

    /* Receive interrupts when pin does up.
     */
    os_memclear(&iprm, sizeof(iprm));
    iprm.int_handler_func = tcd1304_cam_1_igc_end;
    iprm.flags = PINS_INT_RISING;
    pin_gpio_attach_interrupt(&camext.igc_loopback_pin, &iprm);
}

/* Camera interface (structure with function pointers, polymorphism)
 */
const pinsCameraInterface pins_tcd1304_camera_iface
= {
    tcd1304_cam_initialize,
    tcd1304_cam_enumerate_cameras,
    tcd1304_cam_open,
    tcd1304_cam_close,
    tcd1304_cam_start,
    tcd1304_cam_stop,
    tcd1304_cam_set_parameter,
    tcd1304_cam_get_parameter,
    OS_NULL
};

#endif
