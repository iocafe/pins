/**

  @file    common/pins_tcd1304_camera.c
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
#include "pinsx.h"
#if PINS_CAMERA == PINS_TDC1304_CAMERA

#define TDC1304_DATA_CLOCK_HZ 250000.0
#define TDC1304_MIN_DATA_CLOCKS 4000.0
#define TDC1304_DATA_SZ 3694

#define TDC1304_START_CLOCK 10
#define TDC1304_IGC_PULSE_CLOCKS 2
#define TDC1304_SH_PULSE_CLOCKS 1

#ifndef TDC1304_MAX_CAMERAS
#define TDC1304_MAX_CAMERAS 1
#endif

#define TCD1304_MAX_PIN_PRM 14

typedef struct
{
    os_short xx;
}
staticCameraState;


static osalStatus tcd1304_cam_open(
    pinsCamera *c,
    const pinsCameraParams *prm)
{
    osalThreadOptParams opt;
    os_int id;

    os_memclear(c, sizeof(pinsCamera));


     /* Create event to trigger the thread.
     */
    c->camera_event = osal_event_create();

    /* Create thread.
     */
    os_memclear(&opt, sizeof(opt));
    opt.priority = OSAL_THREAD_PRIORITY_TIME_CRITICAL;
    opt.thread_name = "mythread";
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


static void tcd1304_cam_task(
    void *prm,
    osalEvent done)
{
    pinsPhoto *photo;
    staticCameraState *cs;
    pinsCamera *c;
    c = (pinsCamera*)prm;

    cs = &cam_state[c->id];
    osal_event_set(done);

    while (!c->stop_thread && osal_go())
    {
        if (osal_event_wait(c->camera_event, 2017) != OSAL_STATUS_TIMEOUT)
        {
        }
    }
}


BEGIN_PIN_INTERRUPT_HANDLER(tcd1304_cam_1_on_timer)
    os_int
        pos,
        sh_pos,
        data_pos,
        igc_start;

    pos = cam_state[ISR_CAM_IX].pos;
    igc_start = cam_state[ISR_CAM_IX].igc_start;
    if (pos >= igc_start)
    {
        if (pos == igc_start)
        {
            osal_event_set(cam_state[ISR_CAM_IX].c->camera_event);
        }
    }

END_PIN_INTERRUPT_HANDLER(tcd1304_cam_1_on_timer)


static void tcd1304_cam_ll_start(
        pinsCamera *c)
{
    pinTimerParams prm;

    os_memclear(&prm, sizeof(prm));
    prm.flags = PIN_TIMER_START;
    prm.int_handler_func = tcd1304_cam_1_on_timer;

    pin_timer_attach_interrupt(c->timer_pin, &prm);
}

