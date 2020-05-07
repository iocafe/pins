/**

  @file    extensions/camera/windows/pins_windows_usb_camera.cpp
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

#include "extensions\camera\windows\ep_usbcamera\videoInput.h"

 #define PINS_USBCAM_MAX_DATA_SZ (3000 * 2000 * 3)
 #define PINS_USBCAM_BUF_SZ (sizeof(iocBrickHdr) + PINS_USBCAM_MAX_DATA_SZ)


/* Wrapper specific extensions to PinsCamera structure
 */
typedef struct PinsCameraExt
{
    os_uchar *buf;
    os_memsz alloc_sz;
    os_int w;
    os_int h;
    os_int bytes_per_pix;
}
PinsCameraExt;


/* Forward referred static functions.
 */
static void usb_cam_stop(
    pinsCamera *c);

static void usb_cam_task(
    void *prm,
    osalEvent done);


/**
****************************************************************************************************

  @brief Initialize global variables for cameras.
  @anchor usb_cam_initialize

  The usb_cam_initialize() clear global variables for the camera(s). This is not needed for
  Windows USB camera, just filler.

  @return  None

****************************************************************************************************
*/
static void usb_cam_initialize(
    void)
{
}


/**
****************************************************************************************************

  @brief Get information about available cameras.
  @anchor usb_cam_enumerate_cameras

  The usb_cam_enumerate_cameras() function returns number of cameras currently available
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
static os_int usb_cam_enumerate_cameras(
    pinsCameraInfo **camera_info)
{
    os_int nro_cameras;
    videoInput *VI = &videoInput::getInstance();
        
    if (camera_info) *camera_info = OS_NULL;
    nro_cameras = VI->listDevices();
    return nro_cameras;
}


/**
****************************************************************************************************

  @brief Open the camera, set it up.
  @anchor usb_cam_open

  The usb_cam_open() sets ip camera for use. This function is called from application trough 
  camera interface pins_usb_camera_iface.open().

  @param   c Pointer to camera structure.
  @return  OSAL_SUCCESS if all is fine. Other return values indicate an error.

****************************************************************************************************
*/
static osalStatus usb_cam_open(
    pinsCamera *c,
    const pinsCameraParams *prm)
{
    os_int camera_nr;

    os_memclear(c, sizeof(pinsCamera));
    c->camera_pin = prm->camera_pin;
    c->callback_func = prm->callback_func;
    c->callback_context = prm->callback_context;
    c->iface = &pins_usb_camera_iface;

    /* We could support two PINS_USBCAM cameras later on, we should check which static camera
       structure is free, etc. Now one only.
     */
    camera_nr = prm->camera_nr;
    c->camera_nr = camera_nr;

    c->ext = (PinsCameraExt*)os_malloc(sizeof(PinsCameraExt), OS_NULL);
    if (c->ext == OS_NULL) {
        return OSAL_STATUS_MEMORY_ALLOCATION_FAILED;
    }
    os_memclear(c->ext, sizeof(PinsCameraExt));

    return OSAL_SUCCESS;
}


/**
****************************************************************************************************

  @brief Close the camera (release resources).
  @anchor usb_cam_close

  The usb_cam_close() stops the video and releases any resources reserved for the camera.
  This function is called from application trough camera interface pins_usb_camera_iface.close().

  @param   c Pointer to camera structure.
  @return  None

****************************************************************************************************
*/
static void usb_cam_close(
    pinsCamera *c)
{
    usb_cam_stop(c);

    if (c->ext)
    {
        if (c->ext->buf) {
            os_free(c->ext->buf, c->ext->alloc_sz);
        }

        os_free(c->ext, sizeof(PinsCameraExt));
        c->ext = OS_NULL;
    }
}


/**
****************************************************************************************************

  @brief Start vido stream.
  @anchor usb_cam_start

  The usb_cam_start() starts the video. This function is called from application trough
  camera interface pins_usb_camera_iface.start().

  @param   c Pointer to camera structure.
  @return  None

****************************************************************************************************
*/
static void usb_cam_start(
    pinsCamera *c)
{
    osalThreadOptParams opt;
    if (c->camera_thread) return;

    /* Create thread that transfers camera frames.
     */
    os_memclear(&opt, sizeof(opt));
    opt.priority = OSAL_THREAD_PRIORITY_LOW;
    opt.thread_name = "usbcam";
    opt.pin_to_core = OS_TRUE;
    opt.pin_to_core_nr = 0;
    c->camera_thread = osal_thread_create(usb_cam_task, c, &opt, OSAL_THREAD_ATTACHED);
}


/**
****************************************************************************************************

  @brief Stop vido stream.
  @anchor usb_cam_stop

  The usb_cam_stop() stops the video. This function is called from application trough
  camera interface pins_usb_camera_iface.stop().

  @param   c Pointer to camera structure.
  @return  None

****************************************************************************************************
*/
static void usb_cam_stop(
    pinsCamera *c)
{
    if (c->camera_thread == OS_NULL) return;

    c->stop_thread = OS_TRUE;
    osal_thread_join(c->camera_thread);
    c->stop_thread = OS_FALSE;
    c->camera_thread = OS_NULL;
}


/**
****************************************************************************************************

  @brief Set value of camera parameter.
  @anchor usb_cam_set_parameter

  The usb_cam_set_parameter() sets value of a camera parameter. This function is called from
  application trough camera interface pins_usb_camera_iface.set_parameter().

  @param   c Pointer to camera structure.
  @param   ix Parameter index, see enumeration pinsCameraParamIx.
  @param   x Parameter value.
  @return  None

****************************************************************************************************
*/
static void usb_cam_set_parameter(
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
            osal_debug_error("usb_cam_set_parameter: Unknown prm");
            return;
    }
}


/**
****************************************************************************************************

  @brief Get value of camera parameter.
  @anchor usb_cam_get_parameter

  The usb_cam_get_parameter() gets value of a camera parameter. This function is called from
  application trough camera interface pins_usb_camera_iface.get_parameter().

  @param   c Pointer to camera structure.
  @param   ix Parameter index, see enumeration pinsCameraParamIx.
  @return  Parameter value.

****************************************************************************************************
*/
static os_long usb_cam_get_parameter(
    pinsCamera *c,
    pinsCameraParamIx ix)
{
    os_long x;
    switch (ix)
    {
        case PINS_CAM_MAX_BUF_SZ:
            x = PINS_USBCAM_BUF_SZ;
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
  @anchor usb_cam_finalize_camera_photo

  The usb_cam_finalize_camera_photo() sets up pinsPhoto structure "photo" to contain the grabbed
  image. Camera API passed photos to application callback with pointer to this photo structure.

  @param   c Pointer to camera structure.
  @param   photo Pointer to photo structure to set up.
  @return  None.

****************************************************************************************************
*/
static void usb_cam_finalize_camera_photo(
    pinsCamera *c)
{
    pinsPhoto photo;
    iocBrickHdr hdr;
    os_uchar *buf;
    os_int alloc_sz, w, h;

    os_memclear(&photo, sizeof(pinsPhoto));
    os_memclear(&hdr, sizeof(iocBrickHdr));
    photo.hdr = &hdr;

    buf = c->ext->buf;
    w = c->ext->w;
    h = c->ext->h;

    photo.iface = c->iface;
    photo.camera = c;
    photo.data = buf;
    photo.byte_w = w * c->ext->bytes_per_pix;
    photo.data_sz = photo.byte_w * h;

    alloc_sz = (os_int)(photo.data_sz + sizeof(iocBrickHdr));
    hdr.alloc_sz[0] = (os_uchar)alloc_sz;
    hdr.alloc_sz[1] = (os_uchar)(alloc_sz >> 8);
    hdr.alloc_sz[2] = (os_uchar)(alloc_sz >> 16);
    hdr.alloc_sz[3] = (os_uchar)(alloc_sz >> 24);

    photo.w = w;
    photo.h = h;
    photo.format = OSAL_RGB24;

    c->callback_func(&photo, c->callback_context);
}

static void usb_cam_stop_event(int deviceID, void *userData)
{
    videoInput *VI = &videoInput::getInstance();
 
    VI->closeDevice(deviceID);
}


/**
****************************************************************************************************

  @brief Make sure that buffer is large enough for image including header.
  @anchor usb_cam_allocate_buffer

  The usb_cam_allocate_buffer() function...

  @param   c Pointer to camera structure.
  @return  None.

****************************************************************************************************
*/
static osalStatus usb_cam_allocate_buffer(
    pinsCamera *c)
{
    os_int sz;
    
    sz = c->ext->w * c->ext->h * c->ext->bytes_per_pix;
    if (sz > c->ext->alloc_sz)
    {
        if (c->ext->buf) {
            os_free(c->ext->buf, c->ext->alloc_sz);
        }

        c->ext->buf = (os_uchar*)os_malloc(sz, &c->ext->alloc_sz);
        if (c->ext->buf == OS_NULL) {
            return OSAL_STATUS_MEMORY_ALLOCATION_FAILED;
        }
        os_memclear(c->ext->buf, c->ext->alloc_sz);
    }

    return OSAL_SUCCESS;
}

/**
****************************************************************************************************

  @brief Thread to process camera data.
  @anchor usb_cam_task

  The usb_cam_task() thread to process data from camera.

  @param   prm Pointer to pinsCamera structure.
  @param   done Even to be set to allow thread which created this one to proceed.
  @return  None.

****************************************************************************************************
*/
static void usb_cam_task(
    void *prm,
    osalEvent done)
{
    pinsCamera *c;
    os_int camera_nr, nro_cameras;

    c = (pinsCamera*)prm;
    osal_event_set(done);

    camera_nr = c->camera_nr;
    videoInput *VI = &videoInput::getInstance();
 
    while (!c->stop_thread && osal_go())
    {
        nro_cameras = VI->listDevices();
        if(camera_nr >= nro_cameras) {
            osal_debug_error_int("usb_cam_task: Camera number too big", camera_nr);
            goto tryagain;
        }

        if(!VI->setupDevice(camera_nr, 640, 480, 60))
        {
            osal_debug_error_int("usb_cam_task: Setting up camera failed", camera_nr);
            goto tryagain;
        }

        VI->setEmergencyStopEvent(camera_nr, NULL, usb_cam_stop_event);

        while (!c->stop_thread && osal_go())
        {
             if(VI->isFrameNew(camera_nr))
             {
                 c->ext->w = VI->getWidth(camera_nr);
                 c->ext->h = VI->getHeight(camera_nr);
                 c->ext->bytes_per_pix = 3;

                 if (usb_cam_allocate_buffer(c)) {
                     VI->closeDevice(camera_nr);
                     goto getout;
                 }

                 VI->getPixels(camera_nr, c->ext->buf);
                 usb_cam_finalize_camera_photo(c);
             }
            os_timeslice();
                   
            /* if(c == 49) 
            {
                CamParametrs CP = VI->getParametrs(i-1);                        
                CP.Brightness.CurrentValue = 128; 
                CP.Brightness.Flag = 1; 
                VI->setParametrs(i - 1, CP);
            }
 
            if(!VI->isDeviceSetup(i - 1))
            {
                break;
            } */
        }
 
        VI->closeDevice(camera_nr);

tryagain:
        os_sleep(100);
    }

getout:;
}

/* Camera interface (structure with function pointers, polymorphism)
 */
const pinsCameraInterface pins_usb_camera_iface
= {
    usb_cam_initialize,
    usb_cam_enumerate_cameras,
    usb_cam_open,
    usb_cam_close,
    usb_cam_start,
    usb_cam_stop,
    usb_cam_set_parameter,
    usb_cam_get_parameter
};

#endif
