/**

  @file    extensions/camera/esp32/pins_esp32_tdc_line_camera.c
  @brief   TCD1304, etc, line camera driver implementation.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    21.4.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#define PINS_OS_INT_HANDLER_HDRS
#include "pins.h"
#if PINS_CAMERA == PINS_TDC1304_CAMERA

#define TDC1304_TIMING_CLOCK_HZ 200000.0
// #define TDC1304_DATA_SZ (3694/2)
#define TDC1304_DATA_SZ 3694

#ifndef TDC1304_MAX_CAMERAS
#define TDC1304_MAX_CAMERAS 1
#endif

#define TCD1304_MAX_PIN_PRM 14

#define PINS_TCD1304_BUF_SZ (sizeof(iocBrickHdr) + TDC1304_DATA_SZ)

typedef struct
{
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
staticCameraState;

static staticCameraState cam_state[TDC1304_MAX_CAMERAS];

/* Forward referred static functions.
 */
static void tcd1304_cam_task(
    void *prm,
    osalEvent done);

static void tcd1304_cam_ll_start(
    pinsCamera *c);

static void tcd1304_cam_ll_stop(
    pinsCamera *c);

static void tcd1304_setup_camera_io_pins(
    pinsCamera *c);


static void tcd1304_cam_initialize(
    void)
{
    os_memclear(cam_state, TDC1304_MAX_CAMERAS * sizeof(staticCameraState));
}


static osalStatus tcd1304_cam_open(
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

    c->integration_us = 2000;
    c->iface = &pins_tcd1304_camera_iface;

    /* We could support two TDC1304 cameras later on, we should check which static camera
       structure is free, etc. Now one only.
     */
    for (id = 0; cam_state[id].c; id++)
    {
        if (id >= TDC1304_MAX_CAMERAS - 1)
        {
            osal_debug_error("tcd1304_cam_open: Maximum number of cameras used");
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
    opt.thread_name = "tcd1304";
    opt.pin_to_core = OS_TRUE;
    opt.pin_to_core_nr = 0;
    c->camera_thread = osal_thread_create(tcd1304_cam_task, c, &opt, OSAL_THREAD_ATTACHED);

    return OSAL_STATUS_FAILED;
}

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

static void tcd1304_cam_start(
    pinsCamera *c)
{
    tcd1304_setup_camera_io_pins(c);
    tcd1304_cam_ll_start(c);
}


static void tcd1304_cam_stop(
    pinsCamera *c)
{
    tcd1304_cam_ll_stop(c);
}


static void tcd1304_cam_set_parameter(
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
            osal_debug_error("tcd1304_cam_set_parameter: Unknown prm");
            return;
    }
}


static os_long tcd1304_cam_get_parameter(
    pinsCamera *c,
    pinsCameraParamIx ix)
{
    os_long x;
    switch (ix)
    {
        case PINS_CAM_MAX_IMAGE_SZ:
            x = PINS_TCD1304_BUF_SZ;
            break;

        default:
            x = -1;
            break;
    }

    return x;
}


static void tcd1304_finalize_camera_photo(
    pinsCamera *c,
    pinsPhoto *photo)
{
    iocBrickHdr *hdr;
    os_uchar *buf;

    os_memclear(photo, sizeof(pinsPhoto));
    buf = cam_state[c->id].buf;
    os_memclear(buf, sizeof(iocBrickHdr));

    photo->iface = c->iface;
    photo->camera = c;
    photo->buf = buf;
    photo->buf_sz = PINS_TCD1304_BUF_SZ;
    hdr = (iocBrickHdr*)buf;
    hdr->format = IOC_BYTE_BRICK;
    hdr->width[0] = (os_uchar)TDC1304_DATA_SZ;
    hdr->width[1] = (os_uchar)(TDC1304_DATA_SZ >> 8);
    hdr->height[0] = 1;
    hdr->buf_sz[0] = hdr->alloc_sz[0] = (os_uchar)PINS_TCD1304_BUF_SZ;
    hdr->buf_sz[1] = hdr->alloc_sz[1] = (os_uchar)(PINS_TCD1304_BUF_SZ >> 8);

    photo->data = buf + sizeof(iocBrickHdr);
    photo->data_sz = TDC1304_DATA_SZ;
    photo->byte_w = TDC1304_DATA_SZ;
    photo->w = TDC1304_DATA_SZ;
    photo->h = 1;
    photo->format = hdr->format;
}


static void tcd1304_cam_task(
    void *prm,
    osalEvent done)
{
    pinsPhoto photo;
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

                    x >>= 4;
                    while (processed_pos < max_pos) {
                        cs->buf[sizeof(iocBrickHdr) + processed_pos++] = x;
                    }
                    xsum += x;
                    xn ++;
                    cs->processed_pos = processed_pos;
                }

                if (pos > TDC1304_DATA_SZ + 30) // + 30 SLACK
                {
                    tcd1304_finalize_camera_photo(c, &photo);
                    c->callback_func(&photo, c->callback_context);

                    cs->frame_ready = OS_TRUE;
                    cs->processed_pos = 0;
                    pin_ll_set(&cs->igc_pin, cs->igc_on_pulse_setting);

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


BEGIN_PIN_INTERRUPT_HANDLER(tcd1304_cam_1_igc_end)
#define ISR_CAM_IX 0
    cam_state[ISR_CAM_IX].start_new_frame = OS_TRUE;
#undef ISR_CAM_IX
END_PIN_INTERRUPT_HANDLER(tcd1304_cam_1_igc_end)


BEGIN_TIMER_INTERRUPT_HANDLER(tcd1304_cam_1_on_timer)
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

#if PINS_SIMULATED_INTERRUPTS
    if (cam_state[ISR_CAM_IX].pos == TDC1304_DATA_SZ + 50)
    {
        tcd1304_cam_1_igc_end();
    }
#endif

#undef ISR_CAM_IX
END_TIMER_INTERRUPT_HANDLER(tcd1304_cam_1_on_timer)


static void tcd1304_cam_ll_start(
    pinsCamera *c)
{
    staticCameraState *cs;
    pinTimerParams prm;

    os_memclear(&prm, sizeof(prm));
    prm.int_handler_func = tcd1304_cam_1_on_timer;

    pin_timer_attach_interrupt(c->timer_pin, &prm);

    cs = &cam_state[c->id];
    cs->pos = 0;
    cs->processed_pos = 0;
    cs->start_new_frame = OS_FALSE;
    cs->frame_ready = OS_FALSE;
    pin_ll_set(&cs->igc_pin, cs->igc_on_pulse_setting);
}

static void tcd1304_cam_ll_stop(
        pinsCamera *c)
{
}

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


static void tcd1304_setup_camera_io_pins(
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
    clocks_per_sh = os_round_int(0.000001 * c->integration_us * TDC1304_TIMING_CLOCK_HZ);
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

    cs->igc_on_pulse_setting = max_pulse - igc_pulse_setting;
    cs->igc_off_pulse_setting = max_pulse;

    timer_nr = pin_get_prm(c->camera_pin, PIN_TIMER_SELECT);

    /* Integration time (electronic shutter) signal SH.
     */
    cs->sh_prm_count = 0;
    tcd1304_append_pin_parameter(cs->sh_pin_prm, &cs->sh_prm_count, PIN_RV, PIN_RV);
    tcd1304_append_pin_parameter(cs->sh_pin_prm, &cs->sh_prm_count, PIN_TIMER_SELECT, timer_nr);
    tcd1304_append_pin_parameter(cs->sh_pin_prm, &cs->sh_prm_count, PIN_FREQENCY, sh_frequency_hz);
    tcd1304_append_pin_parameter(cs->sh_pin_prm, &cs->sh_prm_count, PIN_RESOLUTION, bits);
    tcd1304_append_pin_parameter(cs->sh_pin_prm, &cs->sh_prm_count, PIN_INIT, sh_pulse_setting);
    tcd1304_append_pin_parameter(cs->sh_pin_prm, &cs->sh_prm_count, PIN_HPOINT, sh_delay_setting);

    cs->sh_pin.type = PIN_PWM;
    cs->sh_pin.bank = pin_get_prm(c->camera_pin, PIN_B_BANK); /* PWM channel */
    cs->sh_pin.addr = pin_get_prm(c->camera_pin, PIN_B);
    cs->sh_pin.prm = cs->sh_pin_prm;
    cs->sh_pin.prm_n = (os_char)cs->sh_prm_count;
    pin_ll_setup(&cs->sh_pin, PINS_DEFAULT);

    /* IGC parameters
     */
    cs->igc_prm_count = 0;
    tcd1304_append_pin_parameter(cs->igc_pin_prm, &cs->igc_prm_count, PIN_RV, PIN_RV);
    tcd1304_append_pin_parameter(cs->igc_pin_prm, &cs->igc_prm_count, PIN_TIMER_SELECT, timer_nr);
    tcd1304_append_pin_parameter(cs->igc_pin_prm, &cs->igc_prm_count, PIN_FREQENCY, sh_frequency_hz);
    tcd1304_append_pin_parameter(cs->igc_pin_prm, &cs->igc_prm_count, PIN_RESOLUTION, bits);
    tcd1304_append_pin_parameter(cs->igc_pin_prm, &cs->igc_prm_count, PIN_INIT, cs->igc_on_pulse_setting);
    tcd1304_append_pin_parameter(cs->igc_pin_prm, &cs->igc_prm_count, PIN_HPOINT, igc_pulse_setting);

    /* Integration clear (new photo) signal IGC.
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
    tcd1304_append_pin_parameter(cs->igc_loopback_pin_prm, &cs->igc_loopback_prm_count, PIN_RV, PIN_RV);
    tcd1304_append_pin_parameter(cs->igc_loopback_pin_prm, &cs->igc_loopback_prm_count, PIN_INTERRUPT_ENABLED, 1);

    /* Integration clear (new photo) signal IGC.
     */
    cs->igc_loopback_pin.type = PIN_INPUT;
    cs->igc_loopback_pin.addr = pin_get_prm(c->camera_pin, PIN_D);
    cs->igc_loopback_pin.prm = cs->igc_loopback_pin_prm;
    cs->igc_loopback_pin.prm_n = (os_char)cs->igc_loopback_prm_count;
#if PINS_SIMULATED_INTERRUPTS
    cs->igc_loopback_pin.int_conf = &cs->loopback_int_conf;
#endif
    pin_ll_setup(&cs->igc_loopback_pin, PINS_DEFAULT);

    /* Receive interrupts when pin does up.
     */
    os_memclear(&iprm, sizeof(iprm));
    iprm.int_handler_func = tcd1304_cam_1_igc_end;
    iprm.flags = PINS_INT_RISING;
    pin_gpio_attach_interrupt(&cs->igc_loopback_pin, &iprm);
}


const pinsCameraInterface pins_tcd1304_camera_iface
= {
    tcd1304_cam_initialize,
    tcd1304_cam_open,
    tcd1304_cam_close,
    tcd1304_cam_start,
    tcd1304_cam_stop,
    tcd1304_cam_set_parameter,
    tcd1304_cam_get_parameter
};

#endif
