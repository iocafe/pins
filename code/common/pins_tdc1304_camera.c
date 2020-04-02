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
// #include "Arduino.h"

// TaskHandle_t complexHandlerTask;
// hw_timer_t * adcTimer = NULL; // our timer

#define TDC1304_DATA_SZ 3800

#ifndef TDC1304_MAX_CAMERAS
#define TDC1304_MAX_CAMERAS 1
#endif

typedef struct
{
    pinsCamera *c;
    volatile os_int pos;
    os_char buf[TDC1304_DATA_SZ];
    os_int data_start, data_end;
    os_int igc_start, igc_end;
    os_int sh_start, sh_end;

    /** Signal input and timing output pins.
     */
    Pin in_pin, igc_pin, sh_pin;
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
    osalThreadOptParams opt;
    os_int id;

    os_memclear(c, sizeof(pinsCamera));
    c->camera_pin = prm->camera_pin;
    c->timer_pin = prm->timer_pin;
    c->callback_func = prm->callback_func;
    c->callback_context = prm->callback_context;

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
    /* tdc1304_cam_ll_stop(c);
    if (was running) tdc1304_cam_ll_start(c); */
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
    pinsCamera *c;
    c = (pinsCamera*)prm;
    osal_event_set(done);

    while (!c->stop_thread)
    {
        osal_event_wait(c->camera_event, 2017);
    }
}


BEGIN_PIN_INTERRUPT_HANDLER(tdc1304_cam_1_on_timer)
#define ISR_CAM_IX 0
    os_int pos, i;
    pos = cam_state[ISR_CAM_IX].pos;

    if (pos >= cam_state[ISR_CAM_IX].data_end)
    {
        if (pos == cam_state[ISR_CAM_IX].data_end)
        {
            cam_state[ISR_CAM_IX].pos = ++pos;
            osal_event_set(cam_state[ISR_CAM_IX].c->camera_event);
        }
    }

    else
    {
        if (pos == cam_state[ISR_CAM_IX].igc_start)
        {
            pin_ll_set(&cam_state[ISR_CAM_IX].igc_pin, 1);
        }
        else if (pos == cam_state[ISR_CAM_IX].igc_end)
        {
            pin_ll_set(&cam_state[ISR_CAM_IX].igc_pin, 0);
        }
        if (pos == cam_state[ISR_CAM_IX].sh_start)
        {
            pin_ll_set(&cam_state[ISR_CAM_IX].sh_pin, 1);
        }
        else if (pos == cam_state[ISR_CAM_IX].sh_end)
        {
            pin_ll_set(&cam_state[ISR_CAM_IX].sh_pin, 0);
        }

        if (pos >= cam_state[ISR_CAM_IX].data_start)
        {
            i = pos - cam_state[ISR_CAM_IX].data_start;
            // cam_state[ISR_CAM_IX].buf[i] = local_adc1_read(ADC1_CHANNEL_0);
        }

        cam_state[ISR_CAM_IX].pos = ++pos;
    }
#undef ISR_CAM_IX
END_PIN_INTERRUPT_HANDLER(tdc1304_cam_1_on_timer)


static void tdc1304_cam_ll_start(
        pinsCamera *c)
{
    // pin_setup_timer(pin, prm);
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
