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
#include <malloc.h>

#include "extensions\camera\windows\ep_usbcamera\videoInput.h"


#define TESTSUM_N 20

/* Wrapper specific extensions to PinsCamera structure
 */
typedef struct PinsCameraExt
{
    os_uchar *buf;
    os_memsz alloc_sz;
    os_int w;
    os_int h;
    os_int bytes_per_pix;

    os_int prm[PINS_NRO_CAMERA_PARAMS];
    os_timer prm_timer;
    volatile os_boolean prm_changed;
    volatile os_boolean reconfigure_camera;

    os_ulong testsum[TESTSUM_N];
}
PinsCameraExt;


/* Forward referred static functions.
 */
static void usb_cam_stop(
    pinsCamera *c);

static void usb_cam_check_image_dims(
    pinsCamera *c);

static void usb_cam_task(
    void *prm,
    osalEvent done);

static void usb_cam_set_parameters(
    pinsCamera *c,
    videoInput *VI,
    os_int camera_nr);


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
    os_int i;

    os_memclear(c, sizeof(pinsCamera));
    c->camera_pin = prm->camera_pin;
    c->callback_func = prm->callback_func;
    c->callback_context = prm->callback_context;
    c->iface = &pins_usb_camera_iface;

    c->ext = (PinsCameraExt*)os_malloc(sizeof(PinsCameraExt), OS_NULL);
    if (c->ext == OS_NULL) {
        return OSAL_STATUS_MEMORY_ALLOCATION_FAILED;
    }
    os_memclear(c->ext, sizeof(PinsCameraExt));
    for (i = 0; i < PINS_NRO_CAMERA_PARAMS; i++) {
        c->ext->prm[i] = -1;
    }

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

  @brief Set camera parameter.
  @anchor usb_cam_set_parameter

  The usb_cam_set_parameter() sets value of a camera parameter. This function is called from
  application trough camera interface pins_usb_camera_iface.set_parameter().

  @param   c Pointer to camera structure.
  @param   ix Parameter index, see enumeration pinsCameraParamIx.
  @param   x Parameter value. -1 to indicate that parameter has not value.
  @return  None

****************************************************************************************************
*/
static void usb_cam_set_parameter(
    pinsCamera *c,
    pinsCameraParamIx ix,
    os_long x)
{
    osal_debug_assert(c);
    if (ix < 0 || ix >= PINS_NRO_CAMERA_PARAMS || x < 0) return;
    if ((os_int)c->ext->prm[ix] == x) return;
    c->ext->prm[ix] = (os_int)x;
    os_get_timer(&c->ext->prm_timer);

    switch (ix)
    {
        case PINS_CAM_NR:
        case PINS_CAM_FRAMERATE:
            c->ext->reconfigure_camera = OS_TRUE;
            break;

        case PINS_CAM_IMG_WIDTH:
        case PINS_CAM_IMG_HEIGHT:
            usb_cam_check_image_dims(c);
            c->ext->reconfigure_camera = OS_TRUE;
            break;

        default:
            break;
    }

    c->ext->prm_changed = OS_TRUE;
}


/**
****************************************************************************************************

  @brief Check that image dimensions are valid for camera.
  @anchor usb_cam_check_image_dims

  The usb_cam_check_image_dims() makes sure that image width is legimate and selects closest
  supported image width. Then image height is forced to match the image width. If multiple image
  heights are supported for given image with, one matching closest to current height setting
  is selected.

  @param   c Pointer to camera structure.
  @return  None

****************************************************************************************************
*/
static void usb_cam_check_image_dims(
    pinsCamera *c)
{
    os_int w, h;

    w = c->ext->prm[PINS_CAM_IMG_WIDTH];
    if (w <= 160) { w = 160; h = 120; }
    else if (w <= 320) { w = 320; h = 240; }
    else if (w <= 640) { w = 640; h = 480; }
    else if (w <= 800) { w = 800; h = 600; }
    else if (w <= 1024) { w = 1024; h = 768; }
    else if (w <= 1280) { w = 1280; h = 800; }
    else { w = 1920; h = 1080; }
    c->ext->prm[PINS_CAM_IMG_WIDTH] = w;
    c->ext->prm[PINS_CAM_IMG_HEIGHT] = h;
}


/**
****************************************************************************************************

  @brief Get camera parameter.
  @anchor usb_cam_get_parameter

  The usb_cam_get_parameter() gets value of a camera parameter. This function is called from
  application trough camera interface pins_usb_camera_iface.get_parameter().

  @param   c Pointer to camera structure.
  @param   ix Parameter index, see enumeration pinsCameraParamIx.
  @return  Parameter value, -1 to indicate that parameter is not set or ix is out of range.

****************************************************************************************************
*/
static os_long usb_cam_get_parameter(
    pinsCamera *c,
    pinsCameraParamIx ix)
{
    osal_debug_assert(c);
    if (ix < 0 || ix >= PINS_NRO_CAMERA_PARAMS) return -1;
    return c->ext->prm[ix];
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
static osalStatus usb_cam_finalize_camera_photo(
    pinsCamera *c)
{
    pinsPhoto photo;
    iocBrickHdr hdr;
    os_int alloc_sz, w, h;

    os_memclear(&photo, sizeof(pinsPhoto));
    os_memclear(&hdr, sizeof(iocBrickHdr));
    photo.hdr = &hdr;

    w = c->ext->w;
    h = c->ext->h;

    photo.iface = c->iface;
    photo.camera = c;
    photo.data = c->ext->buf;
    photo.byte_w = w * c->ext->bytes_per_pix;
    photo.format = OSAL_RGB24;
    photo.data_sz = photo.byte_w * (size_t)h;

#if 1
    os_int y, h2, count, i;
    os_uchar *top, *bottom, u, *p;
    os_ulong testsum = 1234;

    /* BGR - RGB flip (RGB24 format).
     */
    for (y = 0; y<h; y++) {
        p = photo.data + photo.byte_w * (size_t)y;
        count = w;
        while (count --) {
            u = p[0];
            p[0] = p[2];
            p[2] = u;
            testsum += *(os_uint*)p;
            p += 3;
        }
    }

    /* The USB camera can return same image multiple times and
       can get stuck showing one photo, workaround: detect and restart
     */
    for (i = TESTSUM_N - 1; i > 0; i--) {
        c->ext->testsum[i] = c->ext->testsum[i-1];
    }
    c->ext->testsum[0] = testsum;
    if (testsum == c->ext->testsum[1])
    {
        for (i = 1; i < TESTSUM_N; i++) {
            if (testsum != c->ext->testsum[i]) break;
        }

        if (i > TESTSUM_N/2) {
            os_sleep(100);
        }

        if (i >= TESTSUM_N && !c->ext->prm_changed) {
            os_get_timer(&c->ext->prm_timer);
            c->ext->reconfigure_camera = OS_TRUE;
            c->ext->prm_changed = OS_TRUE;
        }

        return OSAL_NOTHING_TO_DO;
    }

    /* This can be done here or by VI->getPixels()
       First flip image, top to bottom.
     */
    os_uchar *tmp = (os_uchar*)_alloca(photo.byte_w);
    h2 = h/2;
    for (y = 0; y<h2; y++) {
        top = photo.data + photo.byte_w * (size_t)y;
        bottom = photo.data + photo.byte_w * (size_t)(h - y - 1);
        os_memcpy(tmp, top, photo.byte_w);
        os_memcpy(top, bottom, photo.byte_w);
        os_memcpy(bottom, tmp, photo.byte_w);
    }
#endif

    alloc_sz = (os_int)(photo.data_sz + sizeof(iocBrickHdr));
    hdr.alloc_sz[0] = (os_uchar)alloc_sz;
    hdr.alloc_sz[1] = (os_uchar)(alloc_sz >> 8);
    hdr.alloc_sz[2] = (os_uchar)(alloc_sz >> 16);
    hdr.alloc_sz[3] = (os_uchar)(alloc_sz >> 24);

    photo.w = w;
    photo.h = h;

    c->callback_func(&photo, c->callback_context);
    return OSAL_SUCCESS;
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
    os_int camera_nr, nro_cameras, w, h, fr;

    c = (pinsCamera*)prm;
    osal_event_set(done);

    videoInput *VI = &videoInput::getInstance();

    while (!c->stop_thread && osal_go())
    {
        camera_nr = c->ext->prm[PINS_CAM_NR] - 1;
        if (camera_nr < 0) camera_nr = 0;
        c->camera_nr = camera_nr;

        nro_cameras = VI->listDevices();
        if(camera_nr >= nro_cameras) {
            osal_debug_error_int("usb_cam_task: Camera number too big", camera_nr);
            goto tryagain;
        }

        w = c->ext->prm[PINS_CAM_IMG_WIDTH];
        h = c->ext->prm[PINS_CAM_IMG_HEIGHT];
        fr = c->ext->prm[PINS_CAM_FRAMERATE];
        if (fr <= 0) fr = 30;
        if(!VI->setupDevice(camera_nr, w, h, fr))
        {
            osal_debug_error_int("usb_cam_task: Setting up camera failed", camera_nr);
            goto tryagain;
        }

        usb_cam_set_parameters(c, VI, camera_nr);

        VI->setEmergencyStopEvent(camera_nr, NULL, usb_cam_stop_event);
        c->ext->reconfigure_camera = OS_FALSE;
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

                 /* Two last arguments are correction of RedAndBlue flipping
                    flipRedAndBlue and vertical flipping flipImage
                  */
#if 1
                 VI->getPixels(camera_nr, c->ext->buf, false, false);
#else
                 VI->getPixels(camera_nr, c->ext->buf, true, true);
#endif
                 if (usb_cam_finalize_camera_photo(c)) {
                    os_timeslice();
                 }
            }
            os_timeslice();

            if(!VI->isDeviceSetup(camera_nr)) {
                break;
            }

            if (c->ext->prm_changed) {
                if (os_has_elapsed(&c->ext->prm_timer, 50))
                {
                    if (c->ext->reconfigure_camera) {
                        break;
                    }
                    usb_cam_set_parameters(c, VI, camera_nr);
                }
            }
        }

        if (VI->isDeviceSetup(camera_nr))
        {
            VI->closeDevice(camera_nr);
        }

tryagain:
        os_sleep(100);
    }

getout:;
}


/**
****************************************************************************************************

  @brief Write parameters to camera driver.
  @anchor usb_cam_set_parameters

  The usb_cam_set_parameters() function....

  See VideoProcAmpProperty Enumeration for description of property values.
  https://docs.microsoft.com/en-us/windows/win32/api/strmif/ne-strmif-videoprocampproperty

  @param   c Pointer to pinsCamera structure.
  @param   VI Video input device.
  @return  None.

****************************************************************************************************
*/
static void usb_cam_set_parameters(
    pinsCamera *c,
    videoInput *VI,
    os_int camera_nr)
{
#define PINCAM_SETPRM_MACRO(a, b, f) \
    x = c->ext->prm[b]; \
    delta = CP.a.Max - CP.a.Min; \
    if (delta > 0) \
    { \
        if (x < 0) x = 50; \
        y = (os_int)(0.01 * delta * x + 0.5); \
        if (y < CP.a.Min) y = CP.a.Min; \
        if (y > CP.a.Max) y = CP.a.Max; \
        CP.a.Flag = f;  /* 1 = KSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO */  \
                        /* 2 = KSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL */  \
        CP.a.CurrentValue = y; \
    }

    os_int x, y;
    os_double delta;

    c->ext->prm_changed = OS_FALSE;

    CamParametrs CP = VI->getParametrs(camera_nr);

    /* Brightness */
    PINCAM_SETPRM_MACRO(Brightness, PINS_CAM_BRIGHTNESS, 2)

    /* Saruration */
    PINCAM_SETPRM_MACRO(Saturation, PINS_CAM_SATURATION, 2)

    /* Focus  */
    CP.Focus.Flag = 2; /* KSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL */
    CP.Focus.CurrentValue = 100;

    /* White balance */
    CP.WhiteBalance.Flag = 1; /* KSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO */
    CP.WhiteBalance.CurrentValue = 1;

    VI->setParametrs(camera_nr, CP);
    if(!VI->isDeviceSetup(camera_nr))
    {
    }
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
