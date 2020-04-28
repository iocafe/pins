/**

  @file    extensions/camera/windows/pins_windows_usb_camera.c
  @brief   Windows USB camera wrapper.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    28.4.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "pinsx.h"
#if PINS_CAMERA == PINS_USB_CAMERA



#ifndef PINS_ESPCAM_MAX_CAMERAS
#define PINS_ESPCAM_MAX_CAMERAS 1
#endif

// #define TCD1304_MAX_PIN_PRM 14

#define PINS_ESPCAM_MAX_DATA_SZ (800 * 600 * 3)
#define PINS_ESPCAM_BUF_SZ (sizeof(iocBrickHdr) + PINS_ESPCAM_MAX_DATA_SZ)

typedef struct
{
    pinsCamera *c;
/*     volatile os_short pos;
    volatile os_short processed_pos;

    os_uchar buf[PINS_ESPCAM_BUF_SZ];

    volatile os_boolean start_new_frame;
    volatile os_boolean frame_ready;

*/
    /* Pin parameters.
     */
/*     os_ushort sh_prm_count;
    os_ushort sh_pin_prm[TCD1304_MAX_PIN_PRM];
*/
    /** Signal input and timing output pins.
     */
//    Pin stribe_pin, trig_pin;
}
staticCameraState;

static staticCameraState cam_state[PINS_ESPCAM_MAX_CAMERAS];




/* Forward referred static functions.
 */
static void esp32_cam_task(
    void *prm,
    osalEvent done);


/**
****************************************************************************************************

  @brief Initialize global variables for cameras.
  @anchor esp32_cam_initialize

  The esp32_cam_initialize() clear global variables for the camera(s). This is necessary to
  when running in microcontroller which doesn't clear memory during soft reboot.

  @return  None

****************************************************************************************************
*/
static void esp32_cam_initialize(
    void)
{
    os_memclear(cam_state, PINS_ESPCAM_MAX_CAMERAS * sizeof(staticCameraState));
}


/**
****************************************************************************************************

  @brief Open the camera, set it up.
  @anchor esp32_cam_open

  The esp32_cam_open() sets ip camera for use. It creates threads, events, etc, nedessary
  for the camera. This function is called from  application trough camera interface
  pins_esp32_camera_iface.open().

  @param   c Pointer to the pin structure for the camera.
  @return  None

****************************************************************************************************
*/
static osalStatus esp32_cam_open(
    pinsCamera *c,
    const pinsCameraParams *prm)
{
    staticCameraState *cs;
    osalThreadOptParams opt;
    os_int id;

    os_memclear(c, sizeof(pinsCamera));
    c->camera_pin = prm->camera_pin;
    c->callback_func = prm->callback_func;
    c->callback_context = prm->callback_context;

    c->integration_us = 2000;
    c->iface = &pins_usb_camera_iface;

    /* We could support two PINS_ESPCAM cameras later on, we should check which static camera
       structure is free, etc. Now one only.
     */
    for (id = 0; cam_state[id].c; id++)
    {
        if (id >= PINS_ESPCAM_MAX_CAMERAS - 1)
        {
            osal_debug_error("esp32_cam_open: Maximum number of cameras used");
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
    c->camera_thread = osal_thread_create(esp32_cam_task, c, &opt, OSAL_THREAD_ATTACHED);

    return OSAL_STATUS_FAILED;
}


/**
****************************************************************************************************

  @brief Close the camera (release resources).
  @anchor esp32_cam_close

  The esp32_cam_close() stops the video and releases any resources reserved for the camera.
  This function is called from  application trough camera interface pins_esp32_camera_iface.close().

  @param   c Pointer to the pin structure for the camera.
  @return  None

****************************************************************************************************
*/
static void esp32_cam_close(
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
  @anchor esp32_cam_start

  The esp32_cam_start() starts the video. This function is called from  application trough
  camera interface pins_esp32_camera_iface.start().

  @param   c Pointer to the pin structure for the camera.
  @return  None

****************************************************************************************************
*/
static void esp32_cam_start(
    pinsCamera *c)
{
    // staticCameraState *cs;

    // cs = &cam_state[c->id];
/*     cs->pos = 0;
    cs->processed_pos = 0;
    cs->start_new_frame = OS_FALSE;
    cs->frame_ready = OS_FALSE;
    pin_ll_set(&cs->igc_pin, cs->igc_on_pulse_setting); */
}


/**
****************************************************************************************************

  @brief Stop vido stream.
  @anchor esp32_cam_stop

  The esp32_cam_stop() stops the video. This function is called from  application trough
  camera interface pins_esp32_camera_iface.stop().

  @param   c Pointer to the pin structure for the camera.
  @return  None

****************************************************************************************************
*/
static void esp32_cam_stop(
    pinsCamera *c)
{
}


/**
****************************************************************************************************

  @brief Set value of camera parameter.
  @anchor esp32_cam_set_parameter

  The esp32_cam_set_parameter() sets value of a camera parameter. This function is called from
  application trough camera interface pins_esp32_camera_iface.set_parameter().

  @param   c Pointer to the pin structure for the camera.
  @param   ix Parameter index, see enumeration pinsCameraParamIx.
  @param   x Parameter value.
  @return  None

****************************************************************************************************
*/
static void esp32_cam_set_parameter(
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
            osal_debug_error("esp32_cam_set_parameter: Unknown prm");
            return;
    }
}


/**
****************************************************************************************************

  @brief Get value of camera parameter.
  @anchor esp32_cam_get_parameter

  The esp32_cam_get_parameter() gets value of a camera parameter. This function is called from
  application trough camera interface pins_esp32_camera_iface.get_parameter().

  @param   c Pointer to the pin structure for the camera.
  @param   ix Parameter index, see enumeration pinsCameraParamIx.
  @return  Parameter value.

****************************************************************************************************
*/
static os_long esp32_cam_get_parameter(
    pinsCamera *c,
    pinsCameraParamIx ix)
{
    os_long x;
    switch (ix)
    {
        case PINS_CAM_MAX_IMAGE_SZ:
            x = PINS_ESPCAM_BUF_SZ;
            break;

        default:
            x = -1;
            break;
    }

    return x;
}


/**
****************************************************************************************************

  @brief Set up "pinsPhoto" structure.
  @anchor tcd1304_finalize_camera_photo

  The tcd1304_finalize_camera_photo() sets up pinsPhoto structure "photo" to contain the grabbed
  image. Camera API passed photos to application callback with pointer to this photo structure.

  @param   c Pointer to the pin structure for the camera.
  @param   photo Pointer to photo structure to set up.
  @return  None.

****************************************************************************************************
*/
#if 0
static void tcd1304_finalize_camera_photo(
    pinsCamera *c,
    pinsPhoto *photo)
{
    iocBrickHdr *hdr;
    os_uchar *buf;

    os_memclear(photo, sizeof(pinsPhoto));
buf = OS_NULL;
    // buf = cam_state[c->id].buf;
    // os_memclear(buf, sizeof(iocBrickHdr));

    photo->iface = c->iface;
    photo->camera = c;
    photo->buf = buf;
    photo->buf_sz = PINS_ESPCAM_BUF_SZ;
    hdr = (iocBrickHdr*)buf;
    hdr->format = IOC_BYTE_BRICK;
    hdr->width[0] = (os_uchar)PINS_ESPCAM_MAX_DATA_SZ;
    hdr->width[1] = (os_uchar)(PINS_ESPCAM_MAX_DATA_SZ >> 8);
    hdr->height[0] = 1;
    hdr->buf_sz[0] = hdr->alloc_sz[0] = (os_uchar)PINS_ESPCAM_BUF_SZ;
    hdr->buf_sz[1] = hdr->alloc_sz[1] = (os_uchar)(PINS_ESPCAM_BUF_SZ >> 8);

    photo->data = buf + sizeof(iocBrickHdr);
    photo->data_sz = PINS_ESPCAM_MAX_DATA_SZ;
    photo->byte_w = PINS_ESPCAM_MAX_DATA_SZ;
    photo->w = PINS_ESPCAM_MAX_DATA_SZ;
    photo->h = 1;
    photo->format = hdr->format;
}
#endif


/**
****************************************************************************************************

  @brief Thread to process camera data.
  @anchor esp32_cam_task

  The esp32_cam_task() is high priority thread to process data from camera.

  @param   prm Pointer to pinsCamera structure.
  @param   done Even to be set to allow thread which created this one to proceed.
  @return  None.

****************************************************************************************************
*/
static void esp32_cam_task(
    void *prm,
    osalEvent done)
{
    // pinsPhoto photo;
    //  staticCameraState *cs;
    pinsCamera *c;

    c = (pinsCamera*)prm;
    // cs = &cam_state[c->id];

    osal_event_set(done);

    while (!c->stop_thread && osal_go())
    {
        if (osal_event_wait(c->camera_event, 2017) != OSAL_STATUS_TIMEOUT)
        {
            /*    if (pos > PINS_ESPCAM_MAX_DATA_SZ + 30) // + 30 SLACK
                {
                    tcd1304_finalize_camera_photo(c, &photo);
                    c->callback_func(&photo, c->callback_context);

                    cs->frame_ready = OS_TRUE;
                    cs->processed_pos = 0;
                    pin_ll_set(&cs->igc_pin, cs->igc_on_pulse_setting);

                }
            }
            */
        }
    }
}


/* Camera interface (structure with function pointers, polymorphism)
 */
const pinsCameraInterface pins_usb_camera_iface
= {
    esp32_cam_initialize,
    esp32_cam_open,
    esp32_cam_close,
    esp32_cam_start,
    esp32_cam_stop,
    esp32_cam_set_parameter,
    esp32_cam_get_parameter
};

#endif
