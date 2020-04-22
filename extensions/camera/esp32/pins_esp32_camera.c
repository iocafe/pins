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
#if PINS_CAMERA == PINS_WROVER_KIT_CAMERA || \
    PINS_CAMERA == PINS_ESP_EYE_CAMERA || \
    PINS_CAMERA == PINS_M5STACK_PSRAM_CAMERA || \
    PINS_CAMERA == PINS_M5STACK_WIDE_CAMERA || \
    PINS_CAMERA == PINS_AI_THINKER_CAMERA

#include "esp_camera.h"
#include <Arduino.h>

//WROVER-KIT PIN Map
#define CAM_PIN_PWDN    -1 //power down is not used
#define CAM_PIN_RESET   -1 //software reset will be performed
#define CAM_PIN_XCLK    21
#define CAM_PIN_SIOD    26
#define CAM_PIN_SIOC    27

#define CAM_PIN_D7      35
#define CAM_PIN_D6      34
#define CAM_PIN_D5      39
#define CAM_PIN_D4      36
#define CAM_PIN_D3      19
#define CAM_PIN_D2      18
#define CAM_PIN_D1       5
#define CAM_PIN_D0       4
#define CAM_PIN_VSYNC   25
#define CAM_PIN_HREF    23
#define CAM_PIN_PCLK    22


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
    os_memclear(cam_state, TDC1304_MAX_CAMERAS * sizeof(staticCameraState));
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
    c->timer_pin = prm->timer_pin;
    c->callback_func = prm->callback_func;
    c->callback_context = prm->callback_context;

    c->integration_us = 2000;
    c->iface = &pins_esp32_camera_iface;

    /* We could support two TDC1304 cameras later on, we should check which static camera
       structure is free, etc. Now one only.
     */
    for (id = 0; cam_state[id].c; id++)
    {
        if (id >= TDC1304_MAX_CAMERAS - 1)
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
    staticCameraState *cs;
    pinTimerParams prm;

    tcd1304_setup_camera_io_pins(c);

    os_memclear(&prm, sizeof(prm));
    prm.int_handler_func = esp32_cam_1_on_timer;

    pin_timer_attach_interrupt(c->timer_pin, &prm);

    cs = &cam_state[c->id];
    cs->pos = 0;
    cs->processed_pos = 0;
    cs->start_new_frame = OS_FALSE;
    cs->frame_ready = OS_FALSE;
    pin_ll_set(&cs->igc_pin, cs->igc_on_pulse_setting);
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
            x = PINS_TCD1304_BUF_SZ;
            break;

        default:
            x = -1;
            break;
    }

    return x;
}


/**
****************************************************************************************************

  @brief Set up pinsPhoto structure.
  @anchor tcd1304_finalize_camera_photo

  The tcd1304_finalize_camera_photo() sets up pinsPhoto structure "photo" to contain the grabbed
  image. Camera API passed photos to application callback with pointer to this photo structure.

  @param   c Pointer to the pin structure for the camera.
  @param   photo Pointer to photo structure to set up.
  @return  None.

****************************************************************************************************
*/
static void tcd1304_finalize_camera_photo(
    pinsCamera *c,
    pinsPhoto *photo)
{
    iocBrickHdr *hdr;
    os_uchar *buf;

    os_memclear(photo, sizeof(pinsPhoto));
    buf = cam_state[c->id].buf;
    os_memclear(buf, sizeof(iocBrickHdr));

    photo->iface = c->iface;
    photo->camera = c;
    photo->buf = buf;
    photo->buf_sz = PINS_TCD1304_BUF_SZ;
    hdr = (iocBrickHdr*)buf;
    hdr->format = IOC_BYTE_BRICK;
    hdr->width[0] = (os_uchar)TDC1304_DATA_SZ;
    hdr->width[1] = (os_uchar)(TDC1304_DATA_SZ >> 8);
    hdr->height[0] = 1;
    hdr->buf_sz[0] = hdr->alloc_sz[0] = (os_uchar)PINS_TCD1304_BUF_SZ;
    hdr->buf_sz[1] = hdr->alloc_sz[1] = (os_uchar)(PINS_TCD1304_BUF_SZ >> 8);

    photo->data = buf + sizeof(iocBrickHdr);
    photo->data_sz = TDC1304_DATA_SZ;
    photo->byte_w = TDC1304_DATA_SZ;
    photo->w = TDC1304_DATA_SZ;
    photo->h = 1;
    photo->format = hdr->format;
}


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
    pinsPhoto photo;
    staticCameraState *cs;
    pinsCamera *c;
    os_int x;
    os_short pos, processed_pos, max_pos;

    c = (pinsCamera*)prm;
    cs = &cam_state[c->id];

    osal_event_set(done);

int dummy = 0, xsum = 0, xn = 0;

    while (!c->stop_thread && osal_go())
    {
        if (osal_event_wait(c->camera_event, 2017) != OSAL_STATUS_TIMEOUT)
        {
            if (!cs->frame_ready)
            {
                pos = cs->pos;
                processed_pos = cs->processed_pos;

                if (processed_pos < TDC1304_DATA_SZ)
                {
                    x = pin_ll_get(&cs->in_pin);
                    // x = local_adc1_read_test(cam_state[c->id].in_pin.addr);

                    if (processed_pos == 0) {
                        pin_ll_set(&cs->igc_pin, cs->igc_off_pulse_setting);
                    }

                    max_pos = pos;
                    if (max_pos > TDC1304_DATA_SZ) {
                        max_pos = TDC1304_DATA_SZ;
                    }

                    x >>= 4;
                    while (processed_pos < max_pos) {
                        cs->buf[sizeof(iocBrickHdr) + processed_pos++] = x;
                    }
                    xsum += x;
                    xn ++;
                    cs->processed_pos = processed_pos;
                }

                if (pos > TDC1304_DATA_SZ + 30) // + 30 SLACK
                {
                    tcd1304_finalize_camera_photo(c, &photo);
                    c->callback_func(&photo, c->callback_context);

                    cs->frame_ready = OS_TRUE;
                    cs->processed_pos = 0;
                    pin_ll_set(&cs->igc_pin, cs->igc_on_pulse_setting);

    if (++dummy > 100 && xn) {
//        osal_debug_error_int("HERE average ",  xsum / xn);
//        osal_debug_error_int("HERE n ",  xn);
        dummy = 0;
    }
    xsum = xn = 0;

                }
            }
        }
    }
}


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
    .xclk_freq_hz = 20000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_JPEG,//YUV422,GRAYSCALE,RGB565,JPEG
    .frame_size = FRAMESIZE_UXGA,//QQVGA-QXGA Do not use sizes above QVGA when not JPEG

    .jpeg_quality = 12, //0-63 lower number means higher quality
    .fb_count = 1 //if more than one, i2s runs in continuous mode. Use only with JPEG
};

esp_err_t camera_init(){
    //power up the camera if PWDN pin is defined
    if(CAM_PIN_PWDN != -1){
        pinMode(CAM_PIN_PWDN, OUTPUT);
        digitalWrite(CAM_PIN_PWDN, LOW);
    }

    //initialize the camera
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Camera Init Failed");
        return err;
    }

    return ESP_OK;
}

esp_err_t camera_capture(){
    //acquire a frame
    camera_fb_t * fb = esp_camera_fb_get();
    if (!fb) {
        ESP_LOGE(TAG, "Camera Capture Failed");
        return ESP_FAIL;
    }
    //replace this with your own function
    process_image(fb->width, fb->height, fb->format, fb->buf, fb->len);

    //return the frame buffer back to the driver for reuse
    esp_camera_fb_return(fb);
    return ESP_OK;
}

#if 0
JPEG HTTP Capture
#include "esp_camera.h"
#include "esp_http_server.h"
#include "esp_timer.h"

typedef struct {
        httpd_req_t *req;
        size_t len;
} jpg_chunking_t;

static size_t jpg_encode_stream(void * arg, size_t index, const void* data, size_t len){
    jpg_chunking_t *j = (jpg_chunking_t *)arg;
    if(!index){
        j->len = 0;
    }
    if(httpd_resp_send_chunk(j->req, (const char *)data, len) != ESP_OK){
        return 0;
    }
    j->len += len;
    return len;
}

esp_err_t jpg_httpd_handler(httpd_req_t *req){
    camera_fb_t * fb = NULL;
    esp_err_t res = ESP_OK;
    size_t fb_len = 0;
    int64_t fr_start = esp_timer_get_time();

    fb = esp_camera_fb_get();
    if (!fb) {
        ESP_LOGE(TAG, "Camera capture failed");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    res = httpd_resp_set_type(req, "image/jpeg");
    if(res == ESP_OK){
        res = httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.jpg");
    }

    if(res == ESP_OK){
        if(fb->format == PIXFORMAT_JPEG){
            fb_len = fb->len;
            res = httpd_resp_send(req, (const char *)fb->buf, fb->len);
        } else {
            jpg_chunking_t jchunk = {req, 0};
            res = frame2jpg_cb(fb, 80, jpg_encode_stream, &jchunk)?ESP_OK:ESP_FAIL;
            httpd_resp_send_chunk(req, NULL, 0);
            fb_len = jchunk.len;
        }
    }
    esp_camera_fb_return(fb);
    int64_t fr_end = esp_timer_get_time();
    ESP_LOGI(TAG, "JPG: %uKB %ums", (uint32_t)(fb_len/1024), (uint32_t)((fr_end - fr_start)/1000));
    return res;
}
JPEG HTTP Stream
#include "esp_camera.h"
#include "esp_http_server.h"
#include "esp_timer.h"

#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

esp_err_t jpg_stream_httpd_handler(httpd_req_t *req){
    camera_fb_t * fb = NULL;
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len;
    uint8_t * _jpg_buf;
    char * part_buf[64];
    static int64_t last_frame = 0;
    if(!last_frame) {
        last_frame = esp_timer_get_time();
    }

    res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
    if(res != ESP_OK){
        return res;
    }

    while(true){
        fb = esp_camera_fb_get();
        if (!fb) {
            ESP_LOGE(TAG, "Camera capture failed");
            res = ESP_FAIL;
            break;
        }
        if(fb->format != PIXFORMAT_JPEG){
            bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
            if(!jpeg_converted){
                ESP_LOGE(TAG, "JPEG compression failed");
                esp_camera_fb_return(fb);
                res = ESP_FAIL;
            }
        } else {
            _jpg_buf_len = fb->len;
            _jpg_buf = fb->buf;
        }

        if(res == ESP_OK){
            res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
        }
        if(res == ESP_OK){
            size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);

            res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
        }
        if(res == ESP_OK){
            res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
        }
        if(fb->format != PIXFORMAT_JPEG){
            free(_jpg_buf);
        }
        esp_camera_fb_return(fb);
        if(res != ESP_OK){
            break;
        }
        int64_t fr_end = esp_timer_get_time();
        int64_t frame_time = fr_end - last_frame;
        last_frame = fr_end;
        frame_time /= 1000;
        ESP_LOGI(TAG, "MJPG: %uKB %ums (%.1ffps)",
            (uint32_t)(_jpg_buf_len/1024),
            (uint32_t)frame_time, 1000.0 / (uint32_t)frame_time);
    }

    last_frame = 0;
    return res;
}
BMP HTTP Capture
#include "esp_camera.h"
#include "esp_http_server.h"
#include "esp_timer.h"

esp_err_t bmp_httpd_handler(httpd_req_t *req){
    camera_fb_t * fb = NULL;
    esp_err_t res = ESP_OK;
    int64_t fr_start = esp_timer_get_time();

    fb = esp_camera_fb_get();
    if (!fb) {
        ESP_LOGE(TAG, "Camera capture failed");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    uint8_t * buf = NULL;
    size_t buf_len = 0;
    bool converted = frame2bmp(fb, &buf, &buf_len);
    esp_camera_fb_return(fb);
    if(!converted){
        ESP_LOGE(TAG, "BMP conversion failed");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    res = httpd_resp_set_type(req, "image/x-windows-bmp")
       || httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.bmp")
       || httpd_resp_send(req, (const char *)buf, buf_len);
    free(buf);
    int64_t fr_end = esp_timer_get_time();
    ESP_LOGI(TAG, "BMP: %uKB %ums", (uint32_t)(buf_len/1024), (uint32_t)((fr_end - fr_start)/1000));
    return res;
}

#endif


#endif
