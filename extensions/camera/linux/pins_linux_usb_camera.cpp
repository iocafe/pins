/**

  @file    extensions/camera/linux/pins_linux_usb_camera.cpp
  @brief   Linux USB camera wrapper.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    29.8.2020

  Wrapper to present video4linux device, like USB camera via "pins" camera API calls.

  Missing features:
  - Input selection
  - Camera parameter settings

  Dependencies
  - sudo apt-get install libv4l-dev

  Credits to example code by Mohamed Thalib and Søren Holm. https://github.com/sgh/v4l2


  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "pinsx.h"
#if PINS_CAMERA

#include <malloc.h>
#include <libv4l2.h>
#include <linux/videodev2.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/ioctl.h>


/* For memory mapping.
 */
typedef struct usbcamBuffer {
        void *                  start;
        size_t                  length;
}
usbcamBuffer;

/* Wrapper specific extensions to PinsCamera structure
 */
typedef struct PinsCameraExt
{
    os_uchar *buf;
    os_memsz alloc_sz;
    os_int w;
    os_int h;
    os_int src_bytes_per_line;
    os_int src_bytes_per_pix;
    os_int target_bytes_per_pix;
    os_uchar *callback_data_ptr;

    os_int prm[PINS_NRO_CAMERA_PARAMS];
    os_timer prm_timer;
    volatile os_boolean prm_changed;
    volatile os_boolean reconfigure_camera;

    int fd;

    usbcamBuffer *buffers;
    os_int n_buffers;
    struct v4l2_buffer v4l_buf;
}
PinsCameraExt;


/* Forward referred static functions.
 */
static void usb_cam_stop(
    pinsCamera *c);

static osalStatus usb_cam_allocate_buffer(
    PinsCameraExt *ext);

static void usb_cam_check_image_dims(
    pinsCamera *c);

static void usb_cam_task(
    void *prm,
    osalEvent done);

static void usb_cam_set_parameters(
    pinsCamera *c);


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
    if (camera_info) *camera_info = OS_NULL;
    return 1;
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
    c->ext->fd = -1;
    c->ext->target_bytes_per_pix = 3;

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
    osal_debug_assert(c != OS_NULL);
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
    osal_debug_assert(c != OS_NULL);
    if (ix < 0 || ix >= PINS_NRO_CAMERA_PARAMS) return -1;
    return c->ext->prm[ix];
}


/**
****************************************************************************************************

  @brief Set up "pinsPhoto" structure.
  @anchor usb_cam_do_photo_callback

  The usb_cam_do_photo_callback() sets up pinsPhoto structure "photo" to contain the grabbed
  image. Camera API passed photos to application callback with pointer to this photo structure.

  @param   c Pointer to camera structure.
  @param   photo Pointer to photo structure to set up.
  @return  OSAL_SUCCESS if all is fine.

****************************************************************************************************
*/
static osalStatus usb_cam_do_photo_callback(
    pinsCamera *c,
    os_uchar *ptr,
    os_memsz bytes)
{
    PinsCameraExt *ext;
    pinsPhoto photo;
    iocBrickHdr hdr;
    os_int alloc_sz, w, h;

    ext = c->ext;
    ext->callback_data_ptr = ptr;

    os_memclear(&photo, sizeof(pinsPhoto));
    os_memclear(&hdr, sizeof(iocBrickHdr));
    photo.hdr = &hdr;

    usb_cam_allocate_buffer(ext);

    w = ext->w;
    h = ext->h;

    photo.iface = c->iface;
    photo.camera = c;
    photo.data = ext->buf;
    photo.byte_w = w * ext->target_bytes_per_pix;
    photo.format = OSAL_RGB24;
    photo.data_sz = photo.byte_w * (size_t)h;

    if (bytes > photo.data_sz) {
        return OSAL_STATUS_FAILED;
    }

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
  @anchor usb_cam_finalize_photo

  The usb_cam_finalize_photo() is called from the application callback function of photo
  is really needed. This is not done in advance, because callbacks for often reject images,
  so we do not want to waste processor time on this.

  This function converts YUV image to RGB, and flips it in memory as "top first".

  @param   photo Pointer to photo structure.
  @return  None.

****************************************************************************************************
*/
static void usb_cam_finalize_photo(
    pinsPhoto *photo)
{
    pinsCamera *c;
    PinsCameraExt *ext;
    os_uchar *d, *s, *simg, *dimg;
    os_int w, h, y, count;
    os_int y1, pb, pr, r, g, b, Y1, Cb, Cr;

    c = photo->camera;
    ext = c->ext;
    simg = ext->callback_data_ptr;
    dimg = photo->data;
    w = ext->w;
    h = ext->h;

    if (ext->target_bytes_per_pix == 3 && ext->src_bytes_per_pix == 2)
    {
        for (y = 0; y < h; y++) {
            s = simg + (h-y-1) * ext->src_bytes_per_line;
            d = dimg + y * photo->byte_w;

            count = w/2;

            while (count--)
            {
                Y1 = s[0];
                Cb = s[1];
                Cr = s[3];
                y1 = (255 * (Y1 -  16)) / 219;
                pb = (255 * (Cb - 128)) / 224;
                pr = (255 * (Cr - 128)) / 224;

                r = y1 + (0    * pb + 1402 * pr)/1000;
                g = y1 + (-344 * pb - 714  * pr)/1000;
                b = y1 + (1772 * pb + 0    * pr)/1000;
                r = r>255?255:r;
                r = r<0?0:r;
                g = g>255?255:g;
                g = g<0?0:g;
                b = b>255?255:b;
                b = b<0?0:b;

                *(d++) = (os_uchar)r;
                *(d++) = (os_uchar)g;
                *(d++) = (os_uchar)b;

                Y1 = s[2];
                Cb = s[1];
                Cr = s[3];
                y1 = (255 * (Y1 -  16)) / 219;
                pb = (255 * (Cb - 128)) / 224;
                pr = (255 * (Cr - 128)) / 224;
                r = y1 + (0    * pb + 1402 * pr)/1000;
                g = y1 + (-344 * pb - 714  * pr)/1000;
                b = y1 + (1772 * pb + 0    * pr)/1000;

                r = r>255?255:r;
                r = r<0?0:r;
                g = g>255?255:g;
                g = g<0?0:g;
                b = b>255?255:b;
                b = b<0?0:b;

                *(d++) = (os_uchar)r;
                *(d++) = (os_uchar)g;
                *(d++) = (os_uchar)b;

                s += 4;
            }
        }
    }
}


/**
****************************************************************************************************

  @brief Make sure that finald buffer for callback buffer is large enough for image including header.
  @anchor usb_cam_allocate_buffer

  The usb_cam_allocate_buffer() function...

  @param   ext Pointer to camera extension structure.
  @return  None.

****************************************************************************************************
*/
static osalStatus usb_cam_allocate_buffer(
    PinsCameraExt *ext)
{
    os_int sz;

    sz = ext->w * ext->h * ext->target_bytes_per_pix;
    if (sz > ext->alloc_sz)
    {
        if (ext->buf) {
            os_free(ext->buf, ext->alloc_sz);
        }

        ext->buf = (os_uchar*)os_malloc(sz, &ext->alloc_sz);
        if (ext->buf == OS_NULL) {
            return OSAL_STATUS_MEMORY_ALLOCATION_FAILED;
        }
        os_memclear(ext->buf, ext->alloc_sz);
    }

    return OSAL_SUCCESS;
}


/**
****************************************************************************************************

  @brief Configure video format and memory maps video buffer.
  @anchor configure_usb_camera

  The configure_usb_camera() function...

  @param   ext Pointer to camera extension structure.
  @return  None.

****************************************************************************************************
*/
osalStatus configure_usb_camera(
    PinsCameraExt *ext)
{
    os_int index, i;

    index = 0;
    if (-1 == ioctl(ext->fd, VIDIOC_S_INPUT, &index)) {
        osal_debug_error("VIDIOC_S_INPUT");
        return OSAL_STATUS_FAILED;
    }

    struct v4l2_capability cap;
    struct v4l2_cropcap cropcap;
    struct v4l2_crop crop;
    struct v4l2_format fmt;
    unsigned int min;

    if (-1 == ioctl (ext->fd, VIDIOC_QUERYCAP, &cap)) {
        if (EINVAL == errno) {
            osal_debug_error("Device is no V4L2 device");
            return OSAL_STATUS_FAILED;
        }
        else {
            osal_debug_error("VIDIOC_QUERYCAP");
            return OSAL_STATUS_FAILED;
        }
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        osal_debug_error("Device is no video capture device");
        return OSAL_STATUS_FAILED;
    }


    if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
        osal_debug_error("Device does not support streaming i/o");
        return OSAL_STATUS_FAILED;
    }

    os_memclear(&cropcap, sizeof(cropcap));

    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (0 == ioctl(ext->fd, VIDIOC_CROPCAP, &cropcap)) {
        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        crop.c = cropcap.defrect; /* reset to default */

        if (-1 == ioctl(ext->fd, VIDIOC_S_CROP, &crop)) {
            switch (errno) {
            case EINVAL:
                /* Cropping not supported. */
                break;
            default:
                /* Errors ignored. */
                break;
            }
        }
    } else {
        /* Errors ignored. */
    }

    os_memclear(&fmt, sizeof(fmt));
    fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width       = ext->prm[PINS_CAM_IMG_WIDTH];
    fmt.fmt.pix.height      = ext->prm[PINS_CAM_IMG_HEIGHT];
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    fmt.fmt.pix.field       = V4L2_FIELD_NONE;

    /* Note VIDIOC_S_FMT may change width and height.
     */
    if (-1 == ioctl (ext->fd, VIDIOC_S_FMT, &fmt)) {
        osal_debug_error("VIDIOC_S_FMT");
        return OSAL_STATUS_FAILED;
    }

    /* Buggy driver paranoia. */
    min = fmt.fmt.pix.width * 2;
    if (fmt.fmt.pix.bytesperline < min)
        fmt.fmt.pix.bytesperline = min;

    min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;

    if (fmt.fmt.pix.sizeimage < min)
        fmt.fmt.pix.sizeimage = min;

    printf("%d %d\n", fmt.fmt.pix.width, fmt.fmt.pix.height);
    printf("%d\n",fmt.fmt.pix.sizeimage);

    ext->w = fmt.fmt.pix.width;
    ext->h = fmt.fmt.pix.height;

    ext->src_bytes_per_line = fmt.fmt.pix.bytesperline;
    if (fmt.fmt.pix.width > 0) {
        ext->src_bytes_per_pix = fmt.fmt.pix.bytesperline / fmt.fmt.pix.width;
    }

    /* Init mmap */
    struct v4l2_requestbuffers req;
    os_memclear(&req, sizeof(req));

    req.count               = 2;
    req.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory              = V4L2_MEMORY_MMAP;

    if (-1 == ioctl(ext->fd, VIDIOC_REQBUFS, &req)) {
        if (EINVAL == errno) {
            osal_debug_error("Device does not support memory mapping");
            return OSAL_STATUS_FAILED;
        } else {
            osal_debug_error("VIDIOC_REQBUFS");
            return OSAL_STATUS_FAILED;
        }
    }

    if (req.count < 2) {
        osal_debug_error("Insufficient buffer memory on device");
        return OSAL_STATUS_FAILED;
    }

    ext->n_buffers = req.count;
    ext->buffers = (usbcamBuffer *)os_malloc(ext->n_buffers * sizeof(usbcamBuffer), OS_NULL);
    if (ext->buffers == OS_NULL) {
        osal_debug_error("Out of memory");
        return OSAL_STATUS_FAILED;
    }
    os_memclear(ext->buffers, ext->n_buffers * sizeof(usbcamBuffer));

    for (i = 0; i < ext->n_buffers; i++)
    {
        struct v4l2_buffer buf;

        os_memclear(&buf, sizeof(buf));

        buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory      = V4L2_MEMORY_MMAP;
        buf.index       = i;

        if (-1 == ioctl (ext->fd, VIDIOC_QUERYBUF, &buf)) {
            osal_debug_error("VIDIOC_QUERYBUF");
            return OSAL_STATUS_FAILED;
        }

        ext->buffers[i].length = buf.length;
        ext->buffers[i].start =
            mmap (NULL /* start anywhere */,
                  buf.length,
                  PROT_READ | PROT_WRITE /* required */,
                  MAP_SHARED /* recommended */,
                  ext->fd, buf.m.offset);

        if (MAP_FAILED == ext->buffers[i].start) {
            osal_debug_error("mmap");
            return OSAL_STATUS_FAILED;
        }
    }

    return OSAL_SUCCESS;
}


/**
****************************************************************************************************

  @brief Release memory mapings for video buffer and close vidro stream's file desctiptor.
  @anchor release_usb_camera_buffers_and_close_fd

  The release_usb_camera_buffers_and_close_fd() function...

  @param   ext Pointer to camera extension structure.
  @return  None.

****************************************************************************************************
*/
static void release_usb_camera_buffers_and_close_fd(
    PinsCameraExt *ext)
{
    os_int i;

    /* Close camera device
     */
    if (ext->fd != -1) {
        close(ext->fd);
        ext->fd = -1;
    }

    if (ext->buffers)
    {
        for (i = 0; i < ext->n_buffers; i++)
        {
            if (ext->buffers[i].start) {
                if (munmap(ext->buffers[i].start, ext->buffers[i].length)) {
                    osal_debug_error("mummap");
                }

                ext->buffers[i].start = OS_NULL;
            }
        }
        os_free(ext->buffers, ext->n_buffers * sizeof(usbcamBuffer));
        ext->buffers = OS_NULL;
    }
    ext->n_buffers = 0;
}


/**
****************************************************************************************************

  @brief Get current video input?
  @anchor get_usb_camera_info

  The get_usb_camera_info() function...

  @param   ext Pointer to camera extension structure.
  @return  None.

****************************************************************************************************
*/
static osalStatus get_usb_camera_info(
    PinsCameraExt *ext)
{
    struct v4l2_input input;
    int index;

    if (-1 == ioctl(ext->fd, VIDIOC_G_INPUT, &index)) {
        osal_debug_error("VIDIOC_G_INPUT");
        return OSAL_STATUS_FAILED;
    }

    os_memclear(&input, sizeof(input));
    input.index = index;

    if (-1 == ioctl(ext->fd, VIDIOC_ENUMINPUT, &input)) {
        osal_debug_error("VIDIOC_ENUMINPUT");
        return OSAL_STATUS_FAILED;
    }

    printf ("Current input: %s\n", input.name);

    return OSAL_SUCCESS;
}


/**
****************************************************************************************************

  @brief Get supported video formats?
  @anchor get_usb_video_info

  The get_usb_video_info() function...

  @param   ext Pointer to camera extension structure.
  @return  None.

****************************************************************************************************
*/
static osalStatus get_usb_video_info(
    PinsCameraExt *ext)
{
    struct v4l2_input input;
    struct v4l2_fmtdesc formats;

    os_memclear(&input, sizeof (input));

    if (-1 == ioctl(ext->fd, VIDIOC_G_INPUT, &input.index)) {
        osal_debug_error("VIDIOC_G_INPUT");
        return OSAL_STATUS_FAILED;
    }

    printf ("Current input %s supports:\n", input.name);

    os_memclear(&formats, sizeof (formats));
    formats.index = 0;
    formats.type  = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    while (0 == ioctl(ext->fd, VIDIOC_ENUM_FMT, &formats)) {
        osal_debug_error_str("format ", (os_char*)formats.description);
        formats.index++;
    }

    if (errno != EINVAL || formats.index == 0) {
        osal_debug_error("VIDIOC_ENUMFMT");
        return OSAL_STATUS_FAILED;
    }

    return OSAL_SUCCESS;
}


/**
****************************************************************************************************

  @brief Prepare USB camera and related stuff for capturing video
  @anchor setup_usb_camera

  The setup_usb_camera() function...

  @param   ext Pointer to camera extension structure.
  @return  None.

****************************************************************************************************
*/
static osalStatus setup_usb_camera(
    PinsCameraExt *ext,
    os_int camera_nr)
{
    os_char buf[32], nbuf[OSAL_NBUF_SZ];
    osalStatus s;

    os_strncpy(buf, "/dev/video", sizeof(buf));
    osal_int_to_str(nbuf, sizeof(nbuf), camera_nr);
    os_strncat(buf, nbuf, sizeof(buf));

    ext->fd = open(buf, O_RDWR);
    if (ext->fd < 0) {
        osal_debug_error_str("opening camera failed: ", buf);
        return OSAL_STATUS_FAILED;
    }

    s = configure_usb_camera(ext);
    if (s) {
        return s;
    }

    s = get_usb_camera_info(ext);
    if (s) {
        return s;
    }

    s = get_usb_video_info(ext);
    if (s) {
        return s;
    }

    return OSAL_SUCCESS;
}


/**
****************************************************************************************************

  @brief Start capturing video
  @anchor start_capturing_video

  The start_capturing_video() function...

  @param   ext Pointer to camera extension structure.
  @return  None.

****************************************************************************************************
*/
static osalStatus start_capturing_video(
    PinsCameraExt *ext)
{
    os_int i;
    enum v4l2_buf_type type;

    for (i = 0; i < ext->n_buffers; ++i) {
        struct v4l2_buffer buf;

        os_memclear(&buf, sizeof(buf));

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (-1 == ioctl (ext->fd, VIDIOC_QBUF, &buf)) {
            osal_debug_error("VIDIOC_QBUF");
            return OSAL_STATUS_FAILED;
        }
    }

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (-1 == ioctl (ext->fd, VIDIOC_STREAMON, &type)) {
        osal_debug_error("VIDIOC_STREAMON");
        return OSAL_STATUS_FAILED;
    }

    return OSAL_SUCCESS;
}

static void stop_capturing_video(
    PinsCameraExt *ext)
{
    enum v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == ioctl (ext->fd, VIDIOC_STREAMOFF, &type)) {
        osal_debug_error("VIDIOC_STREAMOFF");
    }
    os_sleep(100);
}


/**
****************************************************************************************************

  @brief Read one video frame
  @anchor read_video_frame

  The read_video_frame() function reads a video frame from camera, and calls
  usb_cam_do_photo_callback() to convert YUV to RGB, etc, and finally call application
  callback function for received frame

  @param   c Pointer to camera structure.
  @param   ext Pointer to camera extension structure.
  @return  None.

****************************************************************************************************
*/
static osalStatus read_video_frame(
    pinsCamera *c,
    PinsCameraExt *ext)
{
    os_uchar * ptr;
    os_memsz bytes;
    osalStatus s;

    os_memclear(&ext->v4l_buf, sizeof(v4l2_buffer));

    ext->v4l_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ext->v4l_buf.memory = V4L2_MEMORY_MMAP;

    if (-1 == ioctl (ext->fd, VIDIOC_DQBUF, &ext->v4l_buf)) {
        switch (errno) {
        case EAGAIN:
            /* EAGAIN - continue select loop. */
            return OSAL_NOTHING_TO_DO;

        case EIO:
            /* Could ignore EIO, see spec. */

            /* fall through */

        default:
            osal_debug_error("VIDIOC_DQBUF");
            return OSAL_STATUS_FAILED;
        }
    }

    osal_debug_assert(ext->v4l_buf.index < (unsigned)ext->n_buffers);

    ptr = (os_uchar*)ext->buffers[ext->v4l_buf.index].start;
    bytes = ext->v4l_buf.bytesused;
    s = usb_cam_do_photo_callback(c, ptr, bytes);

    if (-1 == ioctl (ext->fd, VIDIOC_QBUF, &ext->v4l_buf)) {
        osal_debug_error("VIDIOC_DQBUF-2");
        return OSAL_STATUS_FAILED;
    }

    return s;
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
    PinsCameraExt *ext;
    os_int camera_nr;
    osalStatus s;
    fd_set fds;
    struct timeval tv;
    int r;

    c = (pinsCamera*)prm;
    osal_event_set(done);
    ext = c->ext;

    while (!c->stop_thread && osal_go())
    {
        camera_nr = ext->prm[PINS_CAM_NR] - 1;
        if (camera_nr < 0) camera_nr = 0;
        c->camera_nr = camera_nr;

        /* Open and setup camera device
         */
        s = setup_usb_camera(ext, camera_nr);
        if (s) goto close_it;

        usb_cam_set_parameters(c);

        s = start_capturing_video(ext);
        if (s) {
            goto close_it;
        }

        c->ext->reconfigure_camera = OS_FALSE;
        while (!c->stop_thread && osal_go())
        {
            FD_ZERO (&fds);
            FD_SET (ext->fd, &fds);

            /* Timeout. */
            tv.tv_sec = 2;
            tv.tv_usec = 0;

            r = select (ext->fd + 1, &fds, NULL, NULL, &tv);

            if (-1 == r) {
                if (EINTR == errno)
                    continue;

                osal_debug_error("ERR select");
            }

            if (0 == r) {
                osal_debug_error("ERR select timeout");
                break;
            }

            s = read_video_frame(c, ext);
            if (s) {
                if (OSAL_IS_ERROR(s)) {
                    break;
                }
                os_timeslice();
            }


            if (c->ext->prm_changed) {
                if (os_has_elapsed(&c->ext->prm_timer, 50))
                {
                    if (c->ext->reconfigure_camera) {
                        break;
                    }
                    usb_cam_set_parameters(c);
                }
            }

static long ulledoo; if (++ulledoo > 10009) {osal_debug_error("ulledoo cam\n"); ulledoo = 0;}
        }

        stop_capturing_video(ext);

close_it:

        /* Close camera device
         */
        release_usb_camera_buffers_and_close_fd(ext);
        os_sleep(300);
    }
}


/**
****************************************************************************************************

  @brief Write parameters to camera driver.
  @anchor usb_cam_set_parameters

  The usb_cam_set_parameters() function....

  Camera parameters not implemented.
  https://docs.microsoft.com/en-us/windows/win32/api/strmif/ne-strmif-videoprocampproperty

  @param   c Pointer to pinsCamera structure.
  @param   VI Video input device.
  @return  None.

****************************************************************************************************
*/
static void usb_cam_set_parameters(
    pinsCamera *c)
{
    OSAL_UNUSED(c);
#if 0
    c->camera_nr
    c->ext->prm[PINS_CAM_BRIGHTNESS]
    c->ext->prm[PINS_CAM_SATURATION]
#endif
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
    usb_cam_get_parameter,
    OS_NULL,
    usb_cam_finalize_photo
};

#endif
