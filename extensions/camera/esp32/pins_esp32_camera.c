/**

  @file    extensions/camera/esp32/pins_esp32_camera.c
  @brief   ESP32-CAM wrapper.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    21.6.2020

  Notes: ESP32 allocates image frame buffer when camera is initalized. It is not practical to
  reallocate this buffer later on, thus camera is first initialized with high resolution and
  quality to get large enough frame buffer. Later code switches to real image size and
  JPEG quality to use.

  Copyright 2020 Espressif, Apace 2 license.
  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#define PINS_OS_INT_HANDLER_HDRS 1
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

static OS_CONST camera_config_t camera_config = {
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

     /* XCLK 20MHz (20000000)or 10MHz (10000000) for OV2640 double FPS (Experimental) */
    .xclk_freq_hz = 10000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_JPEG, /* YUV422,GRAYSCALE,RGB565,JPEG */

    /* CONFIGURE WITH MAXIMUM RESOLUTION AND QUALITY TO SET UP WITH MAX FRAME BUFFER
       SIZE (it will not be resized)
     */
    .frame_size = FRAMESIZE_SVGA,
    .jpeg_quality = 1,  /* 0-63 lower number means higher quality */
    .fb_count = 1       /* if more than one, i2s runs in continuous mode. Use only with JPEG */
};


/* Wrapper specific extensions to PinsCamera structure
 */
typedef struct PinsCameraExt
{
    os_int prm[PINS_NRO_CAMERA_PARAMS];
    os_timer prm_timer;
    volatile os_boolean prm_changed;
    volatile os_boolean enable_interrupts;
    volatile os_boolean camera_paused;

    /* JPEG compression quality 1 - 100. and previous value for change checking.
     */
    volatile os_uchar jpeg_quality;
    os_uchar prev_jpeg_quality;
    int frame_size;
    int prev_frame_size;
}
PinsCameraExt;

static PinsCameraExt camext;


/* Forward referred static functions.
 */
static void esp32_cam_stop(
    pinsCamera *c);

static void esp32_cam_check_dims_and_set_frame_size(
    pinsCamera *c);

static void esp32_cam_task(
    void *prm,
    osalEvent done);

static void esp32_cam_set_parameters(
    void);

static void esp32_cam_global_interrupt_control(
    os_boolean enable,
    void *context);


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
    os_int i;

    os_memclear(c, sizeof(pinsCamera));
    c->camera_pin = prm->camera_pin;
    c->callback_func = prm->callback_func;
    c->callback_context = prm->callback_context;
    c->iface = &pins_esp32_camera_iface;
    c->ext = &camext;

    os_memclear(&camext, sizeof(PinsCameraExt));
    camext.jpeg_quality = 12;
    for (i = 0; i < PINS_NRO_CAMERA_PARAMS; i++) {
        camext.prm[i] = -1;
    }

    camext.enable_interrupts = osal_add_interrupt_to_list(esp32_cam_global_interrupt_control, c);

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
    opt.pin_to_core_nr = 1; // 7.8.2020 test core 1 HERE works a lot faster than 0 for ESP32
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
  @param   x Parameter value. -1 to indicate that parameter has not value.
  @return  None

****************************************************************************************************
*/
static void esp32_cam_set_parameter(
    pinsCamera *c,
    pinsCameraParamIx ix,
    os_long x)
{
    if (ix < 0 || ix >= PINS_NRO_CAMERA_PARAMS || x < 0) return;
    if ((os_int)camext.prm[ix] == x) return;
    camext.prm[ix] = (os_int)x;

    switch (ix)
    {
        case PINS_CAM_IMG_WIDTH:
        case PINS_CAM_IMG_HEIGHT:
            esp32_cam_check_dims_and_set_frame_size(c);
            break;

        default:
            break;
    }

    os_get_timer(&camext.prm_timer);
    camext.prm_changed = OS_TRUE;
}


/**
****************************************************************************************************

  @brief Check that image dimensions are valid for camera and select frame size.
  @anchor esp32_cam_check_dims_and_set_frame_size

  The esp32_cam_check_dims_and_set_frame_size() makes sure that image width is legimate and
  selects closest supported image width. Then image height is forced to match the image width.
  If multiple image heights are supported for given image with, one matching closest to current
  height setting is selected.
  The function also stores matching frame size in global camera_config structure.

  Note: Camera supprts higher resolutions FRAMESIZE_SXGA (1280x1024) and FRAMESIZE_UXGA
  (1600x1200), but current implementation needs more memory than regular ESP32 CAM has.
  Thus these modes are not supported.

  @param   c Pointer to camera structure.
  @return  None

****************************************************************************************************
*/
static void esp32_cam_check_dims_and_set_frame_size(
    pinsCamera *c)
{
    os_int w, h;

    w = camext.prm[PINS_CAM_IMG_WIDTH];
    if (w <= 320) { w = 320; h = 240; camext.frame_size = FRAMESIZE_QVGA; }
    else if (w <= 640) { w = 640; h = 480; camext.frame_size = FRAMESIZE_VGA; }
    else if (w <= 800) { w = 800; h = 600; camext.frame_size = FRAMESIZE_SVGA; }
    else { w = 1024; h = 768; camext.frame_size = FRAMESIZE_XGA; }
    camext.prm[PINS_CAM_IMG_WIDTH] = w;
    camext.prm[PINS_CAM_IMG_HEIGHT] = h;
}


/**
****************************************************************************************************

  @brief Get value of camera parameter.
  @anchor esp32_cam_get_parameter

  The esp32_cam_get_parameter() gets value of a camera parameter. This function is called from
  application trough camera interface pins_esp32_camera_iface.get_parameter().

  @param   c Pointer to camera structure.
  @param   ix Parameter index, see enumeration pinsCameraParamIx.
  @return  Parameter value, -1 to indicate that parameter is not set or ix is out of range.

****************************************************************************************************
*/
static os_long esp32_cam_get_parameter(
    pinsCamera *c,
    pinsCameraParamIx ix)
{
    if (ix < 0 || ix >= PINS_NRO_CAMERA_PARAMS) return -1;
    return camext.prm[ix];
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
    camext.jpeg_quality = quality;
}


/**
****************************************************************************************************

  @brief Set up "pinsPhoto" structure.
  @anchor esp32_cam_photo_complete

  The esp32_cam_photo_complete() sets up pinsPhoto structure "photo" to contain the grabbed
  image. Camera API passed photos to application callback with pointer to this photo structure.

  Known ESP32 camera pixel formats: PIXFORMAT_RGB565, PIXFORMAT_YUV422, PIXFORMAT_GRAYSCALE,
  PIXFORMAT_JPEG, PIXFORMAT_RGB888, PIXFORMAT_RAW, PIXFORMAT_RGB444, PIXFORMAT_RGB555.

  @param   c Pointer to camera structure.
  @param   photo Pointer to photo structure to set up.
  @return  None.

****************************************************************************************************
*/
static void esp32_cam_photo_complete(
    pinsCamera *c,
    camera_fb_t *fb)
{
    pinsPhoto photo;
    iocBrickHdr hdr;
    os_int alloc_sz, w, h;

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
            photo.compression = IOC_JPEG;

            /* Return image quality used to compress the image for adapting compression.
             */
            if (camext.prev_jpeg_quality <= 100) {
                photo.compression |= camext.prev_jpeg_quality;
            }
            break;

        case PIXFORMAT_GRAYSCALE:
            photo.format = OSAL_GRAYSCALE8;
            photo.compression = IOC_UNCOMPRESSED;
            break;

        case PIXFORMAT_RGB888:
            photo.format = OSAL_RGB24;
            photo.compression = IOC_UNCOMPRESSED;
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

  @brief Finalize photo data.
  @anchor esp32_cam_finalize_photo

  The esp32_cam_finalize_photo() is called from the application callback function of photo
  is really needed. This is not done in advance, because callbacks for often reject images,
  so we do not want to waste processor time on this.

  @param   photo Pointer to photo structure.
  @return  None.

****************************************************************************************************
*/
static void esp32_cam_finalize_photo(
    pinsPhoto *photo)
{
    OSAL_UNUSED(photo);
}


/**
****************************************************************************************************

  @brief Thread to process camera data.
  @anchor esp32_cam_task

  The esp32_cam_task() thread to process data from camera.

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
    int q;

    c = (pinsCamera*)prm;
    osal_event_set(done);
    os_get_timer(&error_retry_timer);

    while (!c->stop_thread && osal_go())
    {
        if (!camext.enable_interrupts)
        {
            camext.camera_paused = OS_TRUE;
            while (!camext.enable_interrupts &&
                   !c->stop_thread &&
                   osal_go())
            {
                os_timeslice();
            }
            camext.camera_paused = OS_FALSE;
        }
        else if (!initialized)
        {
            if (os_has_elapsed(&error_retry_timer, 1200))
            {
                os_get_timer(&error_retry_timer);

                if (CAM_PIN_PWDN != -1) {
                    pinMode(CAM_PIN_PWDN, OUTPUT);
                    digitalWrite(CAM_PIN_PWDN, LOW);
                    os_sleep(100);
                }

                err = esp_camera_init(&camera_config);

                if (err != ESP_OK) {
                    if (CAM_PIN_PWDN != -1) {
                        pinMode(CAM_PIN_PWDN, OUTPUT);
                        digitalWrite(CAM_PIN_PWDN, LOW);
                    }

                    osal_set_network_state_int(OSAL_NS_DEVICE_INIT_INCOMPLETE, 0, OS_TRUE);
                    osal_debug_error("ESP32 camera init failed");
                    goto goon;
                }

                esp32_cam_set_parameters();
                camext.prev_jpeg_quality = 255;
                camext.prev_frame_size = -1;

                osal_set_network_state_int(OSAL_NS_DEVICE_INIT_INCOMPLETE, 0, OS_FALSE);

                initialized = OS_TRUE;
            }
            else {
                os_sleep(300);
            }
        }
        else
        {
            os_sleep(20);

            if (camext.jpeg_quality != camext.prev_jpeg_quality)
            {
                camext.prev_jpeg_quality = camext.jpeg_quality;
                sens = esp_camera_sensor_get();
                /* 0-63 lower number means higher quality */
                q = 63 * (100 - (int)camext.prev_jpeg_quality) / 100;
osal_debug_error_int("HERE QUALITY ", q)                ;
                sens->set_quality(sens, q);
            }

            if (camext.frame_size != camext.prev_frame_size)
            {
                camext.prev_frame_size = camext.frame_size;
                sens->set_framesize(sens, camext.prev_frame_size);
            }

            fb = esp_camera_fb_get();
            if (fb == OS_NULL) {
                osal_debug_error("ESP32 camera capture failed");
                initialized = OS_FALSE;
            }
            else {
                esp32_cam_photo_complete(c, fb);
                esp_camera_fb_return(fb);
            }

            if (camext.prm_changed) {
                if (os_has_elapsed(&camext.prm_timer, 50))
                {
                    esp32_cam_set_parameters();
                }
            }
        }
goon:;
    }
}


/**
****************************************************************************************************

  @brief Write parameters to camera driver.
  @anchor esp32_cam_set_parameters

  The esp32_cam_set_parameters() function parametrizes the camera. This is more or less
  experimental, I have not found any useful documentation on ESP32CAM parameters.

  @return  None.

****************************************************************************************************
*/
static void esp32_cam_set_parameters(
    void)
{
/* Set parameter, input 0 - 100, output -2 ... 2
 */
#define PINCAM_SETPRM_MACRO_PM2(a, b) \
    x = camext.prm[b]; \
    y = 4 * (x + 10) / 100 - 2; \
    if (x >= 0) {sens->a(sens, y);}  /* -2 to 2 */

/* Set parameter, input 0 - 100, output 0 ... scale
 */
#define PINCAM_SETPRM_MACRO_SCALE(a, b, scale) \
    x = camext.prm[b]; \
    y = scale * (x + 10) / 100; \
    if (x >= 0) {sens->a(sens, y);}  /* 0 to scale */

    os_int x, y;

    camext.prm_changed = OS_FALSE;
    sensor_t * sens = esp_camera_sensor_get();

    /* Always this way. 0 = disable , 1 = enable.
     */
    sens->set_special_effect(sens, 0);
    sens->set_hmirror(sens, 0);
    sens->set_vflip(sens, 0);

    sens->set_brightness(sens, 0);
    sens->set_contrast(sens, 0);

    /* Set color. wb_mode: 0 to 4 - if awb_gain enabled (0 - Auto, 1 - Sunny,
       2 - Cloudy, 3 - Office, 4 - Home)
     */
    PINCAM_SETPRM_MACRO_PM2(set_saturation, PINS_CAM_SATURATION)
    sens->set_whitebal(sens, 1);
    sens->set_awb_gain(sens, 1);
    sens->set_wb_mode(sens, 0);

    sens->set_gainceiling(sens, 1);
    sens->set_gain_ctrl(sens, 0);

    x = camext.prm[PINS_CAM_BRIGHTNESS];
    if (x >= 0)
    {
        sens->set_ae_level(sens, 0);
        sens->set_exposure_ctrl(sens, 0);
        PINCAM_SETPRM_MACRO_SCALE(set_aec_value, PINS_CAM_BRIGHTNESS, 1200)
    }
    else {
        sens->set_aec_value(sens, 300);
        sens->set_exposure_ctrl(sens, 1);
        sens->set_ae_level(sens, 1);
    }

    sens->set_raw_gma(sens, 1);
    sens->set_lenc(sens, 1); /* this may be 0 also */
    sens->set_dcw(sens, 1);

    sens->set_colorbar(sens, 0);

    /* flip it back ?
     */
    if (sens->id.PID == OV3660_PID) {
        sens->set_vflip(sens, 1);
    }
}


/**
****************************************************************************************************

  @brief Global enable/disable interrupts callback for flash writes.
  @anchor esp32_cam_global_interrupt_control

  The esp32_cam_global_interrupt_control() function is callback function from
  global interrupt control. The purpose of global control is to disable interrupts
  when writing to flash. Heare disabling interrupts means stopping the camera
 (is that good enough?)

  @param   enable OS_TRUE to mark that interrupts are enabled globally, or OS_FALSE
           to mark that interrupts are disabled.
  @param   context Pointer to the pin structure, callback context.
  @return  None.

****************************************************************************************************
*/
static void esp32_cam_global_interrupt_control(
    os_boolean enable,
    void *context)
{
    pinsCamera *c;

    if (camext.enable_interrupts != enable)
    {
        camext.enable_interrupts = enable;
        c = (pinsCamera*)context;

        while (!enable &&
               !camext.camera_paused &&
               c->camera_thread)
        {
            os_timeslice();
        }
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
    esp32_cam_set_jpeg_quality,
    esp32_cam_finalize_photo
};

#endif
