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
#include "pins.h"
#if PINS_CAMERA == PINS_TDC1304_CAMERA

#define TDC1304_DATA_CLOCK_HZ 250000.0
#define TDC1304_MIN_DATA_CLOCKS 3900.0
#define TDC1304_DATA_SZ 3200

#define TDC1304_START_CLOCK 10
#define TDC1304_IGC_PULSE_CLOCKS 2
#define TDC1304_SH_PULSE_CLOCKS 1

#ifndef TDC1304_MAX_CAMERAS
#define TDC1304_MAX_CAMERAS 1
#endif

typedef struct
{
    pinsCamera *c;
    volatile os_int pos;
    os_uchar buf[TDC1304_DATA_SZ];
    os_int data_start, data_end;
    os_int igc_start, igc_end;
    os_int sh_start, sh_end;
    os_int clocks_per_sh, clocks_per_frame;

    volatile os_boolean frame_ready;

    /** Signal input and timing output pins.
     */
    Pin in_pin, igc_pin, sh_pin;
}
staticCameraState;

static staticCameraState cam_state[TDC1304_MAX_CAMERAS];

/* Forward referred static functions.
 */
static void tdc1304_calculate_timing(
    pinsCamera *c);

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
    os_memclear(&cam_state[id], sizeof(staticCameraState));
    cam_state[id].c = c;
    c->id = id;
    tdc1304_setup_camera_io_pins(c);

    /* Create event to trigger the thread.
     */
    c->camera_event = osal_event_create();

    /* Create thread that transfers camera frames.
     */
    os_memclear(&opt, sizeof(opt));
    opt.priority = OSAL_THREAD_PRIORITY_HIGH;
    opt.thread_name = "tdc1304";
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
    tdc1304_calculate_timing(c);
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

    tdc1304_calculate_timing(c);
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

static void tdc1304_calculate_timing(
    pinsCamera *c)
{
    staticCameraState *cs;

    os_int
        data_start, data_end,
        igc_start, igc_end,
        sh_start, sh_end,
        clocks_per_sh, nro_sh_pulses,
        clocks_per_frame;

    /* Calculate timing
       - clocks_per_sh: Number of data clock pulses per sh (integration timing) pulse.
       - nro_sh_pulses: number of SH pulses per frame, rounded upwards.
       - clocks_per_frame: Number of data clock pulses per frame.
     */
    clocks_per_sh = os_round_int(0.000001 * c->integration_us * TDC1304_DATA_CLOCK_HZ);
    if (clocks_per_sh < 10) clocks_per_sh = 10;
    nro_sh_pulses = (os_int)(TDC1304_MIN_DATA_CLOCKS / (os_double)clocks_per_sh + 1.0);
    clocks_per_frame = nro_sh_pulses * clocks_per_sh;

    igc_start = TDC1304_START_CLOCK;
    igc_end = igc_start + TDC1304_IGC_PULSE_CLOCKS;
    sh_start = 0;
    sh_end = TDC1304_SH_PULSE_CLOCKS;
    data_start = igc_end + 1;
    data_end = data_start + TDC1304_DATA_SZ;

    cs = &cam_state[c->id];
    cs->igc_start = igc_start;
    cs->igc_end = igc_end;
    cs->sh_start = sh_start;
    cs->sh_end = sh_end;
    cs->data_start = data_start;
    cs->data_end = data_end;
    cs->clocks_per_sh = clocks_per_sh;
    cs->clocks_per_frame = clocks_per_frame;
}

static void tdc1304_cam_task(
    void *prm,
    osalEvent done)
{
    pinsCameraImage *image;
    pinsCamera *c;
    c = (pinsCamera*)prm;

    osal_event_set(done);

    while (!c->stop_thread && osal_go())
    {
        if (osal_event_wait(c->camera_event, 2017) != OSAL_STATUS_TIMEOUT)
        {
            if (cam_state[c->id].frame_ready && c->callback_func)
            {
                cam_state[c->id].frame_ready = OS_FALSE;

                image = tdc1304_cam_allocate_image(c);
                c->callback_func(image, c->callback_context);
            }
        }
    }
}


BEGIN_PIN_INTERRUPT_HANDLER(tdc1304_cam_1_on_timer)
#define ISR_CAM_IX 0
    os_int
        pos,
        sh_pos,
        data_pos,
        igc_start,
        x;

    pos = cam_state[ISR_CAM_IX].pos;

    if (pos >= cam_state[ISR_CAM_IX].data_end)
    {
        if (pos == cam_state[ISR_CAM_IX].data_end)
        {
            cam_state[ISR_CAM_IX].frame_ready = OS_TRUE;
            osal_event_set(cam_state[ISR_CAM_IX].c->camera_event);
        }
        cam_state[ISR_CAM_IX].pos = ++pos;
        if (pos >= cam_state[ISR_CAM_IX].clocks_per_frame)
        {
            cam_state[ISR_CAM_IX].pos = 0;
        }
    }

    else
    {
        igc_start = cam_state[ISR_CAM_IX].igc_start;
        if (pos >= igc_start)
        {
            if (pos == igc_start)
            {
                cam_state[ISR_CAM_IX].frame_ready = OS_FALSE;
                pin_ll_set(&cam_state[ISR_CAM_IX].igc_pin, 1);
            }
            else if (pos == cam_state[ISR_CAM_IX].igc_end)
            {
                pin_ll_set(&cam_state[ISR_CAM_IX].igc_pin, 0);
            }

            sh_pos = (pos - igc_start) % cam_state[ISR_CAM_IX].clocks_per_sh;
            if (sh_pos == cam_state[ISR_CAM_IX].sh_start)
            {
                pin_ll_set(&cam_state[ISR_CAM_IX].sh_pin, 1);
            }
            else if (sh_pos == cam_state[ISR_CAM_IX].sh_end)
            {
                pin_ll_set(&cam_state[ISR_CAM_IX].sh_pin, 0);
            }

            data_pos = pos - cam_state[ISR_CAM_IX].data_start;
            if (data_pos >= 0 && data_pos < TDC1304_DATA_SZ)
            {
                x = pin_ll_get(&cam_state[ISR_CAM_IX].in_pin);
                cam_state[ISR_CAM_IX].buf[data_pos] = x; // local_adc1_read(ADC1_CHANNEL_0);
            }
        }

        cam_state[ISR_CAM_IX].pos = ++pos;
    }
#undef ISR_CAM_IX
END_PIN_INTERRUPT_HANDLER(tdc1304_cam_1_on_timer)


static void tdc1304_cam_ll_start(
        pinsCamera *c)
{
    pinTimerParams prm;

    os_memclear(&prm, sizeof(prm));
    prm.flags = PIN_TIMER_START;
    prm.int_handler_func = tdc1304_cam_1_on_timer;

    pin_setup_timer(c->timer_pin, &prm);
}

static void tdc1304_cam_ll_stop(
        pinsCamera *c)
{
}

static void tdc1304_setup_camera_io_pins(
        pinsCamera *c)
{
    staticCameraState *cs;

    cs = &cam_state[c->id];

    /* Camera analog video signal input.
     */
    cs->in_pin.type = PIN_ANALOG_INPUT;
    cs->in_pin.addr = pin_get_prm(c->camera_pin, PIN_A);

    /* Integration time (electronic shutter) signal SH.
     */
    cs->sh_pin.type = PIN_OUTPUT;
    cs->sh_pin.addr = pin_get_prm(c->camera_pin, PIN_B);

    /* Integration clear (new image) signal IGC.
     */
    cs->igc_pin.type = PIN_OUTPUT;
    cs->igc_pin.addr = pin_get_prm(c->camera_pin, PIN_C);
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
