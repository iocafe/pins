/**

  @file    extensions/camera/linux/pins_linux_raspicam.cpp
  @brief   Raspberry PI camera wrapper.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    28.7.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "pinsx.h"
#ifdef OSAL_LINUX
#if PINS_CAMERA
#ifdef E_OSVER_pi
#include "raspicam/raspicam.h"


#define TESTSUM_N 20

/* Raspberry specific extensions to PinsCamera structure
 */
typedef struct PinsCameraExt
{
    raspicam::RaspiCam *cam;

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

    /* To avoid repeated camera open error messages.
     */
    os_boolean camera_error_reported;
    os_boolean camera_open_failed;
    os_int64 open_fail_timer;
}
PinsCameraExt;

#define TCAM c->ext->cam


/* Forward referred static functions.
 */
static void raspi_cam_stop(
    pinsCamera *c);

static void raspi_cam_check_image_dims(
    pinsCamera *c);

static void raspi_cam_task(
    void *prm,
    osalEvent done);

static void raspi_cam_set_parameters(
    pinsCamera *c,
    os_int camera_nr);


/**
****************************************************************************************************

  @brief Initialize global variables for cameras.
  @anchor raspi_cam_initialize

  The raspi_cam_initialize() clear global variables for the camera(s). This is not needed for
  Windows USB camera, just filler.

  @return  None

****************************************************************************************************
*/
static void raspi_cam_initialize(
    void)
{
}


/**
****************************************************************************************************

  @brief Get information about available cameras.
  @anchor raspi_cam_enumerate_cameras

  The raspi_cam_enumerate_cameras() function returns number of cameras currently available
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
static os_int raspi_cam_enumerate_cameras(
    pinsCameraInfo **camera_info)
{
    if (camera_info) {
        *camera_info = OS_NULL;
    }
    return 1;
}


/**
****************************************************************************************************

  @brief Set up for a Raspberry camera.
  @anchor raspi_cam_open

  The raspi_cam_open() function prepares pinsCamera structure for use.

  @param   c Pointer to camera structure.
  @return  OSAL_SUCCESS if all is fine. Other return values indicate an error.

****************************************************************************************************
*/
static osalStatus raspi_cam_open(
    pinsCamera *c,
    const pinsCameraParams *prm)
{
    os_int i;

    os_memclear(c, sizeof(pinsCamera));
    c->camera_pin = prm->camera_pin;
    c->callback_func = prm->callback_func;
    c->callback_context = prm->callback_context;
    c->iface = &pins_raspi_camera_iface;

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
  @anchor raspi_cam_close

  The raspi_cam_close() stops the video and releases any resources reserved for the camera.
  This function is called from application trough camera interface pins_raspi_camera_iface.close().

  @param   c Pointer to camera structure.
  @return  None

****************************************************************************************************
*/
static void raspi_cam_close(
    pinsCamera *c)
{
    raspi_cam_stop(c);

    if (c->ext) {
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
  @anchor raspi_cam_start

  The raspi_cam_start() function creates thread which transfers camera images.
  This function is called from application trough camera interface pins_raspi_camera_iface.start().

  @param   c Pointer to camera structure.
  @return  None

****************************************************************************************************
*/
static void raspi_cam_start(
    pinsCamera *c)
{
    osalThreadOptParams opt;
    if (c->camera_thread) return;
    os_memclear(&opt, sizeof(opt));
    opt.priority = OSAL_THREAD_PRIORITY_LOW;
    opt.thread_name = "raspicam";
    c->camera_thread = osal_thread_create(raspi_cam_task, c, &opt, OSAL_THREAD_ATTACHED);
}


/**
****************************************************************************************************

  @brief Stop vido stream.
  @anchor raspi_cam_stop

  The raspi_cam_stop() stops the video. This function is called from application trough
  camera interface pins_raspi_camera_iface.stop().

  @param   c Pointer to camera structure.
  @return  None

****************************************************************************************************
*/
static void raspi_cam_stop(
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
  @anchor raspi_cam_set_parameter

  The raspi_cam_set_parameter() sets value of a camera parameter. This function is called from
  application trough camera interface pins_raspi_camera_iface.set_parameter().

  @param   c Pointer to camera structure.
  @param   ix Parameter index, see enumeration pinsCameraParamIx.
  @param   x Parameter value. -1 to indicate that parameter has not value.
  @return  None

****************************************************************************************************
*/
static void raspi_cam_set_parameter(
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
            raspi_cam_check_image_dims(c);
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
  @anchor raspi_cam_check_image_dims

  The raspi_cam_check_image_dims() makes sure that image width is legitimate and selects closest
  supported image width. Then image height is forced to match the image width. If multiple image
  heights are supported for given image with, one matching closest to current height setting
  is selected.

  @param   c Pointer to camera structure.
  @return  None

****************************************************************************************************
*/
static void raspi_cam_check_image_dims(
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
  @anchor raspi_cam_get_parameter

  The raspi_cam_get_parameter() gets value of a camera parameter. This function is called from
  application trough camera interface pins_raspi_camera_iface.get_parameter().

  @param   c Pointer to camera structure.
  @param   ix Parameter index, see enumeration pinsCameraParamIx.
  @return  Parameter value, -1 to indicate that parameter is not set or ix is out of range.

****************************************************************************************************
*/
static os_long raspi_cam_get_parameter(
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
  @anchor raspi_cam_do_photo_callback

  The raspi_cam_do_photo_callback() sets up pinsPhoto structure "photo" to contain the grabbed
  image. Camera API passed photos to application callback with pointer to this photo structure.

  @param   c Pointer to camera structure.
  @param   photo Pointer to photo structure to set up.
  @return  None.

****************************************************************************************************
*/
static osalStatus raspi_cam_do_photo_callback(
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
    switch (c->ext->bytes_per_pix)
    {
        case 1: photo.format = OSAL_GRAYSCALE8; break;
        case 2: photo.format = OSAL_GRAYSCALE16; break;
        case 3: photo.format = OSAL_RGB24; break;
        case 4: photo.format = OSAL_RGBA32; break;

    }

    photo.data_sz = photo.byte_w * (size_t)h;

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

/**
****************************************************************************************************

  @brief Finalize photo data.
  @anchor raspi_cam_finalize_photo

  The raspi_cam_finalize_photo() is called from the application callback function of photo
  is really needed. This is not done in advance, because callbacks for often reject images,
  so we do not want to waste processor time on this.

  This function converts YUV image to RGB, and flips it in memory as "top first".

  @param   photo Pointer to photo structure.
  @return  None.

****************************************************************************************************
*/
static void raspi_cam_finalize_photo(
    pinsPhoto *photo)
{
    pinsCamera *c;
    PinsCameraExt *ext;

    c = photo->camera;
    ext = c->ext;

    ext->cam->retrieve(c->ext->buf);
}


/**
****************************************************************************************************

  @brief Make sure that buffer is large enough for image including header.
  @anchor raspi_cam_allocate_buffer

  The raspi_cam_allocate_buffer() function...

  @param   c Pointer to camera structure.
  @return  None.

****************************************************************************************************
*/
static osalStatus raspi_cam_allocate_buffer(
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
  @anchor raspi_cam_task

  The raspi_cam_task() thread to process data from camera.

  @param   prm Pointer to pinsCamera structure.
  @param   done Even to be set to allow thread which created this one to proceed.
  @return  None.

****************************************************************************************************
*/
static void raspi_cam_task(
    void *prm,
    osalEvent done)
{
    pinsCamera *c;
    os_int camera_nr, sz, w, h, fr;
    os_int bytes_per_line, bytes_per_pix;

    c = (pinsCamera*)prm;
    osal_event_set(done);

    while (!c->stop_thread && osal_go())
    {
        camera_nr = c->ext->prm[PINS_CAM_NR] - 1;
        if (camera_nr < 0) camera_nr = 0;
        c->camera_nr = camera_nr;

        /* If we are in failed state, try again every 5 seconds
         */
        if (c->ext->camera_open_failed) {
            if (!os_has_elapsed(&c->ext->open_fail_timer, 5000)) {
                os_sleep(100);
                continue;
            }
        }

        w = c->ext->prm[PINS_CAM_IMG_WIDTH];
        h = c->ext->prm[PINS_CAM_IMG_HEIGHT];
        fr = c->ext->prm[PINS_CAM_FRAMERATE];
        if (fr <= 0) fr = 30;

        TCAM = new raspicam::RaspiCam;
        raspi_cam_set_parameters(c, camera_nr);
        if (!TCAM->open())
        {
            if (c->ext->camera_error_reported) {
                osal_debug_error("Unable to open raspi camera");
                c->ext->camera_error_reported = OS_TRUE;
            }
            c->ext->camera_open_failed = OS_TRUE;
            os_get_timer(&c->ext->open_fail_timer);
            goto tryagain;
        }
        c->ext->reconfigure_camera = OS_FALSE;
        c->ext->camera_open_failed = OS_TRUE;
        c->ext->camera_error_reported = OS_TRUE;

        while (!c->stop_thread && osal_go())
        {
// static long ulledoo; if (++ulledoo > 10009) {osal_debug_error("ulledoo cam\n"); ulledoo = 0;}

            TCAM->grab();
            w = TCAM->getWidth();
            h = TCAM->getHeight();
            sz = TCAM->getImageBufferSize();
            if (w < 10 || h < 10 || sz < 100) {os_sleep(50); continue;}
            bytes_per_line = (sz+h-1) / h;
            bytes_per_pix = bytes_per_line / w;
            if (bytes_per_pix < 1 || bytes_per_pix > 4) {os_sleep(50); continue;}
            w = bytes_per_line / bytes_per_pix;

            c->ext->w = w;
            c->ext->h = h;
            c->ext->bytes_per_pix = bytes_per_pix;

            if (sz > c->ext->alloc_sz) {
                if (raspi_cam_allocate_buffer(c)) break;
            }

            raspi_cam_do_photo_callback(c);

            if (c->ext->prm_changed) {
                if (os_has_elapsed(&c->ext->prm_timer, 50)) {
                    if (c->ext->reconfigure_camera) {
                        break;
                    }
                    raspi_cam_set_parameters(c, camera_nr);
                }
            }
            os_sleep(20);
        }

tryagain:
        if (TCAM) {
            TCAM->release(); delete TCAM; TCAM = OS_NULL;
        }
        os_sleep(100);
    }

    if (TCAM) {
        TCAM->release(); delete TCAM; TCAM = OS_NULL;
    }
}


/**
****************************************************************************************************

  @brief Write parameters to camera driver.
  @anchor raspi_cam_set_parameters

  The raspi_cam_set_parameters() function....

  See VideoProcAmpProperty Enumeration for description of property values.
  https://docs.microsoft.com/en-us/windows/win32/api/strmif/ne-strmif-videoprocampproperty

  @param   c Pointer to pinsCamera structure.
  @return  None.

****************************************************************************************************
*/
static void raspi_cam_set_parameters(
    pinsCamera *c,
    os_int camera_nr)
{
    os_int w, h;

    w = c->ext->prm[PINS_CAM_IMG_WIDTH];
    if (w > 10 && w < 10000) TCAM->setWidth(w);
    h = c->ext->prm[PINS_CAM_IMG_HEIGHT];
    if (h > 10 && h < 10000) TCAM->setHeight(h);


    /* Set shutter speed us.
     */
    // TCAM->setShutterSpeed(t->exposure_time * 1000.0);

    /* Set color format: RGB or grayscale. (note we use BGR byte order
     * internally within oecore)
     */
    // TCAM->setFormat(t->colorformat==2
    //        ? raspicam::RASPICAM_FORMAT_BGR
    //    : raspicam::RASPICAM_FORMAT_GRAY);

    /* Automatic color balancing always off.
    PINCAM_SETPRM_MACRO(Saturation, PINS_CAM_SATURATION, 2)
    switch (t->whitebalance)
    {
        case OEPI_WHITE_BALANCE_OFF:
            TCAM->setAWB(raspicam::RASPICAM_AWB_OFF);
            break;

        case OEPI_AUTO_WHITE_BALANCE:
            TCAM->setAWB(raspicam::RASPICAM_AWB_AUTO);
            break;
    }
     */

    /* Rotate image 180 degrees if needed.
     */

    // TCAM->setHorizontalFlip ((bool)(t->rotate180));
    TCAM->setVerticalFlip(true);
    TCAM->setHorizontalFlip(true);

    /* Set exposure control.
     *     PINCAM_SETPRM_MACRO(Brightness, PINS_CAM_BRIGHTNESS, 2)

    switch (t->exposurectrl)
    {
        default:
        case OEPI_EXPOSURE_CTRL_OFF:
            TCAM->setExposure(raspicam::RASPICAM_EXPOSURE_OFF);
            break;

        case OEPI_AUTO_EXPOSURE_CTRL:
            TCAM->setExposure(raspicam::RASPICAM_EXPOSURE_AUTO);
        break;
    }
     */
}

/* Camera interface (structure with function pointers, polymorphism)
 */
const pinsCameraInterface pins_raspi_camera_iface
= {
    raspi_cam_initialize,
    raspi_cam_enumerate_cameras,
    raspi_cam_open,
    raspi_cam_close,
    raspi_cam_start,
    raspi_cam_stop,
    raspi_cam_set_parameter,
    raspi_cam_get_parameter,
    OS_NULL,
    raspi_cam_finalize_photo
};

#endif
#endif
#endif