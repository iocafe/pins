/**

  @file    common/pins_tdc1304_camera.c
  @brief   Camera hardware API.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    2.4.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#define PINS_OS_INT_HANDLER_HDRS
#include "pins.h"
#if PINS_CAMERA == PINS_TDC1304_CAMERA

#define TDC1304_DATA_CLOCK_HZ 200000.0
#define TDC1304_DATA_SZ 3694

#ifndef TDC1304_MAX_CAMERAS
#define TDC1304_MAX_CAMERAS 1
#endif

#define TCD1394_MAX_PIN_PRM 14

typedef struct
{
    pinsCamera *c;
    volatile os_short pos;
    volatile os_short processed_pos;

    os_uchar buf[TDC1304_DATA_SZ];

    volatile os_boolean start_new_frame;
    volatile os_boolean frame_ready;

    /* Interrupts are enabled globally. Used to disable interrupts when writing to flash.
     */
    // volatile os_boolean interrupts_enabled;

    os_int igc_on_pulse_setting;
    os_int igc_off_pulse_setting;

    /* Pin parameters.
     */
    os_ushort sh_prm_count;
    os_ushort igc_prm_count;
    os_ushort igc_loopback_prm_count;
    os_ushort sh_pin_prm[TCD1394_MAX_PIN_PRM];
    os_ushort igc_pin_prm[TCD1394_MAX_PIN_PRM];
    os_ushort igc_loopback_pin_prm[TCD1394_MAX_PIN_PRM];

    /** Signal input and timing output pins.
     */
    Pin in_pin, igc_pin, sh_pin, igc_loopback_pin;
}
staticCameraState;


static staticCameraState cam_state[TDC1304_MAX_CAMERAS];

/* Forward referred static functions.
 */
static void tdc1304_cam_task(
    void *prm,
    osalEvent done);

static void tdc1304_cam_ll_start(
    pinsCamera *c);

static void tdc1304_cam_ll_stop(
    pinsCamera *c);

static void tdc1304_setup_camera_io_pins(
    pinsCamera *c);


static void tdc1304_cam_initialize(
    void)
{
    os_memclear(cam_state, TDC1304_MAX_CAMERAS * sizeof(staticCameraState));
}


static osalStatus tdc1304_cam_open(
    pinsCamera *c,
    const pinsCameraParams *prm)
{
    staticCameraState *cs;
    osalThreadOptParams opt;
    os_int id;

    os_memclear(c, sizeof(pinsCamera));
    c->camera_pin = prm->camera_pin;
    c->timer_pin = prm->timer_pin;
    c->callback_func = prm->callback_func;
    c->callback_context = prm->callback_context;

    c->integration_us = 1000;
    c->iface = &pins_tdc1304_camera_iface;

    /* We could support two TDC1304 cameras later on, we should check which static camera
       structure is free, etc. Now one only.
     */
    for (id = 0; cam_state[id].c; id++)
    {
        if (id >= TDC1304_MAX_CAMERAS - 1)
        {
            osal_debug_error("tdc1304_cam_open: Maximum number of cameras used");
            return OSAL_STATUS_FAILED;
        }
    }
    cs = &cam_state[id];
    os_memclear(cs, sizeof(staticCameraState));
    cs->c = c;
    c->id = id;

    /* Create event to trigger the thread.
     */
    c->camera_event = osal_event_create();

    /* Create thread that transfers camera frames.
     */
    os_memclear(&opt, sizeof(opt));
    opt.priority = OSAL_THREAD_PRIORITY_TIME_CRITICAL;
    opt.thread_name = "tdc1304";
    opt.pin_to_core = OS_TRUE;
    opt.pin_to_core_nr = 0;
    c->camera_thread = osal_thread_create(tdc1304_cam_task, c, &opt, OSAL_THREAD_ATTACHED);

    return OSAL_STATUS_FAILED;
}

static void tdc1304_cam_close(
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

static void tdc1304_cam_start(
    pinsCamera *c)
{
    tdc1304_setup_camera_io_pins(c);
    tdc1304_cam_ll_start(c);
}


static void tdc1304_cam_stop(
    pinsCamera *c)
{
    tdc1304_cam_ll_stop(c);
}


static void tdc1304_cam_set_parameter(
    pinsCamera *c,
    pinsCameraParamIx ix,
    os_long x)
{
    switch (ix)
    {
        case PINS_CAM_INTEGRATION_US:
            if (c->integration_us == x) return;
            c->integration_us = x;
            break;

        default:
            osal_debug_error("tdc1304_cam_set_parameter: Unknown prm");
            return;
    }
}

static void tdc1304_cam_release_image(
    pinsCameraImage *image)
{
    os_free(image, sizeof(pinsCameraImage) + TDC1304_DATA_SZ);
}



static pinsCameraImage *tdc1304_cam_allocate_image(
    pinsCamera *c)
{
    pinsCameraImage *image;
    os_memsz buf_sz;

    buf_sz = sizeof(pinsCameraImage) + TDC1304_DATA_SZ;
    image = (pinsCameraImage*)os_malloc(buf_sz, OS_NULL);
    if (image  == OS_NULL)
    {
        osal_debug_error("tdc1304_cam_allocate_image: Memory allocation failed");
        return OS_NULL;
    }
    os_memclear(image, sizeof(pinsCameraImage));

    image->iface = c->iface;
    image->buf = (os_uchar*)image + sizeof(pinsCameraImage);
    os_memcpy(image->buf, cam_state[c->id].buf, TDC1304_DATA_SZ);
    image->buf_sz = TDC1304_DATA_SZ;
    image->byte_w = TDC1304_DATA_SZ;
    image->w = TDC1304_DATA_SZ;
    image->h = 1;

    return image;
}


static void tdc1304_cam_task(
    void *prm,
    osalEvent done)
{
    pinsCameraImage *image;
    staticCameraState *cs;
    pinsCamera *c;
    os_int x;
    os_short pos, processed_pos, max_pos;

    c = (pinsCamera*)prm;
    cs = &cam_state[c->id];

    osal_event_set(done);

int dummy = 0, xsum = 0, xn = 0;

    while (!c->stop_thread && osal_go())
    {
        if (osal_event_wait(c->camera_event, 2017) != OSAL_STATUS_TIMEOUT)
        {
            if (!cs->frame_ready)
            {
                pos = cs->pos;
                processed_pos = cs->processed_pos;

                if (processed_pos < TDC1304_DATA_SZ)
                {
                    x = pin_ll_get(&cs->in_pin);
                    // x = local_adc1_read_test(cam_state[c->id].in_pin.addr);

                    if (processed_pos == 0) {
                        pin_ll_set(&cs->igc_pin, cs->igc_off_pulse_setting);
                    }

                    max_pos = pos;
                    if (max_pos > TDC1304_DATA_SZ) {
                        max_pos = TDC1304_DATA_SZ;
                    }

                    while (processed_pos < max_pos) {
                        cs->buf[processed_pos++] = x;
                    }
                    xsum += x;
                    xn ++;
                    cs->processed_pos = processed_pos;
                }

                if (pos > TDC1304_DATA_SZ + 30) // + 30 SLACK
                {
                    image = tdc1304_cam_allocate_image(c);
                    c->callback_func(image, c->callback_context);

                    cs->frame_ready = OS_TRUE;
                    cs->processed_pos = 0;
                    pin_ll_set(&cs->igc_pin, cs->igc_on_pulse_setting);

    if (++dummy > 100 && xn) {
        osal_debug_error_int("HERE average ",  xsum / xn);
        osal_debug_error_int("HERE n ",  xn);
        dummy = 0;
    }
    xsum = xn = 0;

                }
            }
        }
    }
}


BEGIN_TIMER_INTERRUPT_HANDLER(tdc1304_cam_1_on_timer)
#define ISR_CAM_IX 0
    if (cam_state[ISR_CAM_IX].start_new_frame)
    {
        cam_state[ISR_CAM_IX].pos = 0;
        cam_state[ISR_CAM_IX].processed_pos = 0;
        cam_state[ISR_CAM_IX].start_new_frame = OS_FALSE;
        cam_state[ISR_CAM_IX].frame_ready = OS_FALSE;
    }
    else  {
        cam_state[ISR_CAM_IX].pos++;
    }

    osal_event_set(cam_state[ISR_CAM_IX].c->camera_event);

#undef ISR_CAM_IX
END_TIMER_INTERRUPT_HANDLER(tdc1304_cam_1_on_timer)


BEGIN_PIN_INTERRUPT_HANDLER(tdc1304_cam_1_igc_end)
#define ISR_CAM_IX 0
    cam_state[ISR_CAM_IX].start_new_frame = OS_TRUE;
#undef ISR_CAM_IX
END_PIN_INTERRUPT_HANDLER(tdc1304_cam_1_igc_end)


static void tdc1304_cam_ll_start(
    pinsCamera *c)
{
    staticCameraState *cs;
    pinTimerParams prm;

    os_memclear(&prm, sizeof(prm));
    prm.int_handler_func = tdc1304_cam_1_on_timer;

    pin_timer_attach_interrupt(c->timer_pin, &prm);

    cs = &cam_state[c->id];
    cs->pos = 0;
    cs->processed_pos = 0;
    cs->start_new_frame = OS_FALSE;
    cs->frame_ready = OS_FALSE;
    pin_ll_set(&cs->igc_pin, cs->igc_on_pulse_setting);
}

static void tdc1304_cam_ll_stop(
        pinsCamera *c)
{
}

static void tdc1304_append_pin_parameter(
    os_ushort *pin_prm,
    os_ushort *pin_prm_count,
    pinPrm prm,
    os_int value)
{
    os_ushort i;

    i = *pin_prm_count;
    if (i >= TCD1394_MAX_PIN_PRM - 1)
    {
        osal_debug_error("tcd1394: too many pin parameters");
        return;
    }

    pin_prm[i++] = prm;
    pin_prm[i++] = value;
    *pin_prm_count = i;
}


static void tdc1304_setup_camera_io_pins(
        pinsCamera *c)
{
    staticCameraState *cs;
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

    cs = &cam_state[c->id];

    /* Camera analog video signal input.
     */
    cs->in_pin.type = PIN_ANALOG_INPUT;
    cs->in_pin.addr = pin_get_prm(c->camera_pin, PIN_A);
    pin_ll_setup(&cs->in_pin, PINS_DEFAULT);

    /* Calculate timing
     */
    clocks_per_sh = os_round_int(0.000001 * c->integration_us * TDC1304_DATA_CLOCK_HZ);
    if (clocks_per_sh < 10) clocks_per_sh = 10;
    sh_frequency_hz = os_round_int(TDC1304_DATA_CLOCK_HZ / clocks_per_sh);
    if (sh_frequency_hz < 10) sh_frequency_hz = 10;
    sh_period_us = 1000000.0 / sh_frequency_hz;

    /* Determine bits to have approximately 0.1 us resolution.
     */
    bits = 16;
    while ( sh_period_us / (1 << bits) < 0.1)
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

    cs->igc_on_pulse_setting = max_pulse - igc_pulse_setting;
    cs->igc_off_pulse_setting = max_pulse;

    timer_nr = pin_get_prm(c->camera_pin, PIN_TIMER_SELECT);

    /* Integration time (electronic shutter) signal SH.
     */
    cs->sh_prm_count = 0;
    tdc1304_append_pin_parameter(cs->sh_pin_prm, &cs->sh_prm_count, PIN_RV, PIN_RV);
    tdc1304_append_pin_parameter(cs->sh_pin_prm, &cs->sh_prm_count, PIN_TIMER_SELECT, timer_nr);
    tdc1304_append_pin_parameter(cs->sh_pin_prm, &cs->sh_prm_count, PIN_FREQENCY, sh_frequency_hz);
    tdc1304_append_pin_parameter(cs->sh_pin_prm, &cs->sh_prm_count, PIN_RESOLUTION, bits);
    tdc1304_append_pin_parameter(cs->sh_pin_prm, &cs->sh_prm_count, PIN_INIT, sh_pulse_setting);
    tdc1304_append_pin_parameter(cs->sh_pin_prm, &cs->sh_prm_count, PIN_HPOINT, sh_delay_setting);

    cs->sh_pin.type = PIN_PWM;
    cs->sh_pin.bank = pin_get_prm(c->camera_pin, PIN_B_BANK); /* PWM channel */
    cs->sh_pin.addr = pin_get_prm(c->camera_pin, PIN_B);
    cs->sh_pin.prm = cs->sh_pin_prm;
    cs->sh_pin.prm_n = (os_char)cs->sh_prm_count;
    pin_ll_setup(&cs->sh_pin, PINS_DEFAULT);

    /* IGC parameters
     */
    cs->igc_prm_count = 0;
    tdc1304_append_pin_parameter(cs->igc_pin_prm, &cs->igc_prm_count, PIN_RV, PIN_RV);
    tdc1304_append_pin_parameter(cs->igc_pin_prm, &cs->igc_prm_count, PIN_TIMER_SELECT, timer_nr);
    tdc1304_append_pin_parameter(cs->igc_pin_prm, &cs->igc_prm_count, PIN_FREQENCY, sh_frequency_hz);
    tdc1304_append_pin_parameter(cs->igc_pin_prm, &cs->igc_prm_count, PIN_RESOLUTION, bits);
    tdc1304_append_pin_parameter(cs->igc_pin_prm, &cs->igc_prm_count, PIN_INIT, cs->igc_on_pulse_setting);
    tdc1304_append_pin_parameter(cs->igc_pin_prm, &cs->igc_prm_count, PIN_HPOINT, igc_pulse_setting);

    /* Integration clear (new image) signal IGC.
     */
    cs->igc_pin.type = PIN_PWM;
    cs->igc_pin.bank = pin_get_prm(c->camera_pin, PIN_C_BANK); /* PWM channel */
    cs->igc_pin.addr = pin_get_prm(c->camera_pin, PIN_C);
    cs->igc_pin.prm = cs->igc_pin_prm;
    cs->igc_pin.prm_n = (os_char)cs->igc_prm_count;
    pin_ll_setup(&cs->igc_pin, PINS_DEFAULT);

    /* IGC loop back parameters
     */
    cs->igc_loopback_prm_count = 0;
    tdc1304_append_pin_parameter(cs->igc_loopback_pin_prm, &cs->igc_loopback_prm_count, PIN_RV, PIN_RV);
    tdc1304_append_pin_parameter(cs->igc_loopback_pin_prm, &cs->igc_loopback_prm_count, PIN_INTERRUPT_ENABLED, 1);

    /* Integration clear (new image) signal IGC.
     */
    cs->igc_loopback_pin.type = PIN_INPUT;
    cs->igc_loopback_pin.addr = pin_get_prm(c->camera_pin, PIN_D);
    cs->igc_loopback_pin.prm = cs->igc_loopback_pin_prm;
    cs->igc_loopback_pin.prm_n = (os_char)cs->igc_loopback_prm_count;
    pin_ll_setup(&cs->igc_loopback_pin, PINS_DEFAULT);

    /* Receive interrupts when pin does up.
     */
    os_memclear(&iprm, sizeof(iprm));
    iprm.int_handler_func = tdc1304_cam_1_igc_end;
    iprm.flags = PINS_INT_RISING;
    pin_gpio_attach_interrupt(&cs->igc_loopback_pin, &iprm);
}


const pinsCameraInterface pins_tdc1304_camera_iface
= {
    tdc1304_cam_initialize,
    tdc1304_cam_open,
    tdc1304_cam_close,
    tdc1304_cam_start,
    tdc1304_cam_stop,
    tdc1304_cam_set_parameter,
    tdc1304_cam_release_image
};

#endif
