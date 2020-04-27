/**

  @file    extensions/camera/common/pins_camera.h
  @brief   Camera hardware API.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    21.4.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/

/* Camera types supported by pins libeary
 */
#define PINS_NO_CAMERA 0
#define PINS_TCD1304_CAMERA 1
#define PINS_WROVER_KIT_CAMERA 10
#define PINS_ESP_EYE_CAMERA 11
#define PINS_M5STACK_PSRAM_CAMERA 12
#define PINS_M5STACK_WIDE_CAMERA 13
#define PINS_AI_THINKER_CAMERA 14

/* If no camera defined for build, set 0
 */
#ifndef PINS_CAMERA
#define PINS_CAMERA PINS_NO_CAMERA
#endif

/* If we got a camera
 */
#if PINS_CAMERA

struct pinsCameraInterface;
struct pinsCamera;

/** Camera photo as received by camera callback function.
 */
typedef struct pinsPhoto
{
    /** Camera interface (structure of camera implementation function pointers).
     */
    const struct pinsCameraInterface *iface;

    /** Camera pointer
     */
    struct pinsCamera *camera;

    /** Image buffer and buffer size in bytes. Image buffer starts with
        flat iocBrickHdr followed by photo pixel data.
     */
    os_uchar *buf;
    os_memsz buf_sz;

    /** Image pixel data.
     */
    os_uchar *data;
    os_memsz data_sz;

    /** Image width in bytes, pointer to pixel below p[0] is p[byte_w].
     */
    os_int byte_w;

    /** Image size, w = width in pixels, h = height in pixels.
     */
    os_int w, h;

    /** Image format
     */
    os_uchar format;
}
pinsPhoto;


/** Camera callback function type, called when a frame has been captured.
 */
typedef void pinsCameraCallbackFunc(
    pinsPhoto *photo,
    void *context);


/** Parameters for open() function.
 */
typedef struct pinsCameraParams
{
    /** Pointer to callback function and application specific content pointer to pass
        to the callback function..
     */
    pinsCameraCallbackFunc *callback_func;
    void *callback_context;

    /** Pointer to camera's pin structure.
     */
    const struct Pin *camera_pin;

    /** Pointer to camera timer's pin structure. OS_NULL if not needed.
     */
    const struct Pin *timer_pin;
}
pinsCameraParams;


/** Enumeration of camera parameters which can be modified.

    - PINS_CAM_INTEGRATION_US: Set integration time
    - PINS_CAM_INTEGRATION_US: Get maximum photo size in bytes including brick header

 */
typedef enum pinsCameraParamIx
{
    PINS_CAM_INTEGRATION_US,
    PINS_CAM_MAX_IMAGE_SZ
//     PINS_CAM_FLAGH_NS
}
pinsCameraParamIx;


/** Camera state structure.
 */
typedef struct pinsCamera
{
    /** Camera interface (structure of camera implementation function pointers).
     */
    const struct pinsCameraInterface *iface;

    /** Pointer to callback function and application specific content pointer to pass
        to the callback function..
     */
    pinsCameraCallbackFunc *callback_func;
    void *callback_context;

    /** Thread running the camera, OS_NULL if none.
     */
    osalThread *camera_thread;

    /** Event to trigger camera thread, OS_NULL if none.
     */
    osalEvent camera_event;

    /** Flag to stop camera thread.
     */
    volatile os_boolean stop_thread;

    /** Camera identifier, used internally by implementation.
     */
    os_int id;

    /** Camera parameters.
     */
    os_long integration_us;

    /** Pointer to camera's pin structure.
     */
    const struct Pin *camera_pin;

#if PINS_CAMERA == PINS_TCD1304_CAMERA
    const struct Pin *timer_pin;
#endif
}
pinsCamera;


/**
****************************************************************************************************

  Stream Interface structure.

  The interface structure contains set of function pointers. These function pointers point
  generally to functions which do implemen a specific stream. The functions pointer can also
  point to default implementations in osal_stream.c.

****************************************************************************************************
*/
typedef struct pinsCameraInterface
{
    /* Initialize library code for the camera type.
     */
    void (*initialize)(
        void);

    /* Open a camera.
     */
    osalStatus (*open)(
        pinsCamera *c,
        const pinsCameraParams *prm);

    void (*close)(
        pinsCamera *c);

    void (*start)(
        pinsCamera *c);

    void (*stop)(
        pinsCamera *c);

    /* void (*trig)(
        pinsCamera *c); */

    void (*set_parameter)(
        pinsCamera *c,
        pinsCameraParamIx ix,
        os_long x);

    os_long (*get_parameter)(
        pinsCamera *c,
        pinsCameraParamIx ix);
}
pinsCameraInterface;


#if PINS_CAMERA == PINS_TCD1304_CAMERA
  extern const pinsCameraInterface pins_tcd1304_camera_iface;
  #define PINS_CAMERA_IFACE pins_tcd1304_camera_iface
#endif

#if PINS_CAMERA == PINS_WROVER_KIT_CAMERA || \
    PINS_CAMERA == PINS_ESP_EYE_CAMERA || \
    PINS_CAMERA == PINS_M5STACK_PSRAM_CAMERA || \
    PINS_CAMERA == PINS_M5STACK_WIDE_CAMERA || \
    PINS_CAMERA == PINS_AI_THINKER_CAMERA
  extern const pinsCameraInterface pins_esp32_camera_iface;
  #define PINS_CAMERA_IFACE pins_esp32_camera_iface
#endif


void pins_store_photo_to_brick(
    pinsPhoto *photo,
    iocBrickBuffer *b,
    iocBrickCompression compression);

#endif