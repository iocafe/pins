/**

  @file    extensions/camera/esp32/pins_esp32_camera.c
  @brief   ESP32-CAM wrapper.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    21.4.2020

  Copyright 2020 Espressif, Apace 2 license.
  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "pinsx.h"
#if PINS_IS_ESP32_CAMERA

#ifdef ESP_PLATFORM
#include "esp_camera.h"
#include "Arduino.h"
#endif
#include "extensions/camera/esp32/pins_esp32_camera_pins.h"

/* Alternate pin names to match either example code.
 */
#define CAM_PIN_PWDN    PWDN_GPIO_NUM
#define CAM_PIN_RESET   RESET_GPIO_NUM

#define CAM_PIN_XCLK    XCLK_GPIO_NUM
#define CAM_PIN_SIOD    SIOD_GPIO_NUM
#define CAM_PIN_SIOC    SIOC_GPIO_NUM

#define CAM_PIN_D7      Y9_GPIO_NUM
#define CAM_PIN_D6      Y8_GPIO_NUM
#define CAM_PIN_D5      Y7_GPIO_NUM
#define CAM_PIN_D4      Y6_GPIO_NUM
#define CAM_PIN_D3      Y5_GPIO_NUM
#define CAM_PIN_D2      Y4_GPIO_NUM
#define CAM_PIN_D1      Y3_GPIO_NUM
#define CAM_PIN_D0      Y2_GPIO_NUM

#define CAM_PIN_VSYNC   VSYNC_GPIO_NUM
#define CAM_PIN_HREF    HREF_GPIO_NUM
#define CAM_PIN_PCLK    PCLK_GPIO_NUM

static camera_config_t camera_config = {
    .pin_pwdn  = CAM_PIN_PWDN,
    .pin_reset = CAM_PIN_RESET,
    .pin_xclk = CAM_PIN_XCLK,
    .pin_sscb_sda = CAM_PIN_SIOD,
    .pin_sscb_scl = CAM_PIN_SIOC,

    .pin_d7 = CAM_PIN_D7,
    .pin_d6 = CAM_PIN_D6,
    .pin_d5 = CAM_PIN_D5,
    .pin_d4 = CAM_PIN_D4,
    .pin_d3 = CAM_PIN_D3,
    .pin_d2 = CAM_PIN_D2,
    .pin_d1 = CAM_PIN_D1,
    .pin_d0 = CAM_PIN_D0,
    .pin_vsync = CAM_PIN_VSYNC,
    .pin_href = CAM_PIN_HREF,
    .pin_pclk = CAM_PIN_PCLK,

     //XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
    .xclk_freq_hz = 10000000, // 20000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_JPEG,//YUV422,GRAYSCALE,RGB565,JPEG
    .frame_size = FRAMESIZE_QVGA,//  FRAMESIZE_QVGA. FRAMESIZE_UXGA,//QQVGA-QXGA Do not use sizes above QVGA when not JPEG

    .jpeg_quality = 12, //0-63 lower number means higher quality
    .fb_count = 1 //if more than one, i2s runs in continuous mode. Use only with JPEG
};


#ifndef PINS_ESPCAM_MAX_CAMERAS
#define PINS_ESPCAM_MAX_CAMERAS 1
#endif

// #define esp32_cam_MAX_PIN_PRM 14

#define PINS_ESPCAM_MAX_DATA_SZ (640 * 480 * 3)
#define PINS_ESPCAM_BUF_SZ (sizeof(iocBrickHdr) + PINS_ESPCAM_MAX_DATA_SZ)


/* Forward referred static functions.
 */
static void esp32_cam_stop(pinsCamera *c);
static void esp32_cam_task(void *prm, osalEvent done);


/**
****************************************************************************************************

  @brief Initialize global variables for cameras.
  @anchor esp32_cam_initialize

  The esp32_cam_initialize() clear global variables for the camera(s). So far the function does
  nothing for ESP32 camera.

  @return  None

****************************************************************************************************
*/
static void esp32_cam_initialize(
    void)
{
}


/**
****************************************************************************************************

  @brief Get information about available cameras.
  @anchor esp32_enumerate_cameras

  The esp32_enumerate_cameras() function returns always 1 as number of cameras currently
  available and in future optionally camera information.

  @param   camera_info Where to store pointer to camera info. The argument can be OS_NULL if
           only number of available cameras is needed. The enumerate_cameras function can
           also set the camera_info pointer to OS_NULL if it doesn't provide any camera
           information.
           If information structure is returned, it must be released by calling
           pins_release_camera_info() function.

  @return  Number of available cameras, we return always one.

****************************************************************************************************
*/
static os_int esp32_enumerate_cameras(
    pinsCameraInfo **camera_info)
{
    if (camera_info) *camera_info = OS_NULL;
    return 1;
}


/**
****************************************************************************************************

  @brief Open the camera, set it up.
  @anchor esp32_cam_open

  The esp32_cam_open() sets ip camera for use. This function is called from  application trough
  camera interface pins_esp32_camera_iface.open().

  @param   c Pointer to camera structure.
  @return  OSAL_SUCCESS if all is fine. Other return values indicate an error.

****************************************************************************************************
*/
static osalStatus esp32_cam_open(
    pinsCamera *c,
    const pinsCameraParams *prm)
{
    os_memclear(c, sizeof(pinsCamera));
    c->camera_pin = prm->camera_pin;
    c->callback_func = prm->callback_func;
    c->callback_context = prm->callback_context;
    c->iface = &pins_esp32_camera_iface;
    c->jpeg_quality = 12;
    return OSAL_SUCCESS;
}


/**
****************************************************************************************************

  @brief Close the camera (release resources).
  @anchor esp32_cam_close

  The esp32_cam_close() stops the video and releases any resources reserved for the camera.
  This function is called from application trough camera interface pins_esp32_camera_iface.close().

  @param   c Pointer to camera structure.
  @return  None

****************************************************************************************************
*/
static void esp32_cam_close(
    pinsCamera *c)
{
    esp32_cam_stop(c);
}


/**
****************************************************************************************************

  @brief Start vido stream.
  @anchor esp32_cam_start

  The esp32_cam_start() starts the video. This function is called from application trough
  camera interface pins_esp32_camera_iface.start().

  - Create thread that transfers camera frames.

  @param   c Pointer to camera structure.
  @return  None

****************************************************************************************************
*/
static void esp32_cam_start(
    pinsCamera *c)
{
    osalThreadOptParams opt;
    if (c->camera_thread) return;
    os_memclear(&opt, sizeof(opt));
    opt.priority = OSAL_THREAD_PRIORITY_NORMAL;
    opt.thread_name = "espcam";
    opt.stack_size = 8000;
    opt.pin_to_core = OS_TRUE;
    opt.pin_to_core_nr = 0;
    c->camera_thread = osal_thread_create(esp32_cam_task, c, &opt, OSAL_THREAD_ATTACHED);
}


/**
****************************************************************************************************

  @brief Stop vido stream.
  @anchor esp32_cam_stop

  The esp32_cam_stop() stops the video. This function is called from application trough
  camera interface pins_esp32_camera_iface.stop().

  - Stops thread that transfers camera frames.

  @param   c Pointer to camera structure.
  @return  None

****************************************************************************************************
*/
static void esp32_cam_stop(
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
  @anchor esp32_cam_set_parameter

  The esp32_cam_set_parameter() sets value of a camera parameter. This function is called from
  application trough camera interface pins_esp32_camera_iface.set_parameter().

  @param   c Pointer to camera structure.
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

  @param   c Pointer to camera structure.
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
        case PINS_CAM_MAX_BUF_SZ:
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

  @brief Set JPEG compression quality.
  @anchor esp32_cam_set_jpeg_quality

  The esp32_cam_set_jpeg_quality() function is used when camera driver takes care of JPEG 
  compression and we want to adjust compressed image size (inverse of quality) according
  to brick buffer, etc size. 

  @param   c Pointer to camera structure.
  @param   quality JPEG compression quality 1 - 100.
  @return  None.

****************************************************************************************************
*/
static void esp32_cam_set_jpeg_quality(
    pinsCamera *c,
    os_uchar quality)
{
    c->jpeg_quality = quality;
}


/**
****************************************************************************************************

  @brief Set up "pinsPhoto" structure.
  @anchor esp32_cam_finalize_camera_photo

  The esp32_cam_finalize_camera_photo() sets up pinsPhoto structure "photo" to contain the grabbed
  image. Camera API passed photos to application callback with pointer to this photo structure.

  Known ESP32 camera pixel formats: PIXFORMAT_RGB565, PIXFORMAT_YUV422, PIXFORMAT_GRAYSCALE,
  PIXFORMAT_JPEG, PIXFORMAT_RGB888, PIXFORMAT_RAW, PIXFORMAT_RGB444, PIXFORMAT_RGB555.

  @param   c Pointer to camera structure.
  @param   photo Pointer to photo structure to set up.
  @return  None.

****************************************************************************************************
*/
static void esp32_cam_finalize_camera_photo(
    pinsCamera *c,
    camera_fb_t *fb)
{
    pinsPhoto photo;
    iocBrickHdr hdr;
    os_int alloc_sz, w, h;
    os_uchar quality;

    os_memclear(&photo, sizeof(pinsPhoto));
    os_memclear(&hdr, sizeof(iocBrickHdr));
    photo.hdr = &hdr;

    w = fb->width;
    h = fb->height;

    photo.iface = c->iface;
    photo.camera = c;
    photo.data = fb->buf;
    photo.data_sz = fb->len;

    switch (fb->format)
    {
        default:
        case PIXFORMAT_JPEG:
            photo.format = OSAL_RGB24;
            photo.compression = IOC_NORMAL_JPEG;
            break;

        case PIXFORMAT_GRAYSCALE:
            photo.format = OSAL_GRAYSCALE8;
            photo.compression = IOC_UNCOMPRESSED_BRICK;
            break;

        case PIXFORMAT_RGB888:
            photo.format = OSAL_RGB24;
            photo.compression = IOC_UNCOMPRESSED_BRICK;
            break;
    }
    hdr.compression = (os_uchar)photo.compression;

    photo.byte_w = w * OSAL_BITMAP_BYTES_PER_PIX(photo.format);
    photo.w = w;
    photo.h = h;

    alloc_sz = (os_int)(photo.byte_w * h + sizeof(iocBrickHdr));
    hdr.alloc_sz[0] = (os_uchar)alloc_sz;
    hdr.alloc_sz[1] = (os_uchar)(alloc_sz >> 8);
    hdr.alloc_sz[2] = (os_uchar)(alloc_sz >> 16);
    hdr.alloc_sz[3] = (os_uchar)(alloc_sz >> 24);

    c->callback_func(&photo, c->callback_context);
}


/**
****************************************************************************************************

  @brief Thread to process camera data.
  @anchor esp32_cam_task

  The usb_cam_task() thread to process data from camera.


  ESP32 camera API functions
  - esp_camera_init: Initialize camera.
  - esp_camera_fb_get: Grab a frame.
  - esp_camera_fb_return: Returns the frame buffer back to the driver for reuse.

  @param   prm Pointer to pinsCamera structure.
  @param   done Even to be set to allow thread which created this one to proceed.
  @return  None.

****************************************************************************************************
*/
static void esp32_cam_task(
    void *prm,
    osalEvent done)
{
    pinsCamera *c;
    camera_fb_t *fb;
    sensor_t *sens;
    os_timer error_retry_timer = 0;
    esp_err_t err;
    os_boolean initialized = OS_FALSE;

    c = (pinsCamera*)prm;
    osal_event_set(done);
    os_get_timer(&error_retry_timer);

    while (!c->stop_thread && osal_go())
    {
        if (!initialized)
        {
            if (os_has_elapsed(&error_retry_timer, 1200))
            {
                os_get_timer(&error_retry_timer);

                if (CAM_PIN_PWDN != -1) {
                    pinMode(CAM_PIN_PWDN, OUTPUT);
                    digitalWrite(CAM_PIN_PWDN, LOW);
                }

                err = esp_camera_init(&camera_config);
                if (err != ESP_OK) {
                    osal_debug_error("ESP32 camera init failed");
                    goto goon;
                }

//                sens = esp_camera_sensor_get();
                //initial sensors are flipped vertically and colors are a bit saturated
                /* if (s->id.PID == OV3660_PID) {
                  s->set_vflip(s, 1);//flip it back
                  s->set_brightness(s, 1);//up the blightness just a bit
                  s->set_saturation(s, -2);//lower the saturation
                } */
                //drop down frame size for higher initial frame rate
//                sens->set_framesize(sens, FRAMESIZE_QVGA);

                initialized = OS_TRUE;
            }
            else {
                os_sleep(300);
            }
        }
        else
        {
            os_sleep(20);
            fb = esp_camera_fb_get();
            if (fb == OS_NULL) {
                osal_debug_error("ESP32 camera capture failed");
                initialized = OS_FALSE;
            }
            else {
                esp32_cam_finalize_camera_photo(c, fb);
                esp_camera_fb_return(fb);
            }
        }
goon:;
    }
}

/* Camera interface (structure with function pointers, polymorphism)
 */
const pinsCameraInterface pins_esp32_camera_iface
= {
    esp32_cam_initialize,
    esp32_enumerate_cameras,
    esp32_cam_open,
    esp32_cam_close,
    esp32_cam_start,
    esp32_cam_stop,
    esp32_cam_set_parameter,
    esp32_cam_get_parameter,
    esp32_cam_set_jpeg_quality
};

#endif
