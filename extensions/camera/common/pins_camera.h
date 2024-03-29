/**

  @file    extensions/camera/common/pins_camera.h
  @brief   Camera hardware API.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    26.4.2021

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#pragma once
#ifndef PINS_CAMERA_H_
#define PINS_CAMERA_H_
#include "pinsx.h"

struct iocBrickHdr;
struct iocBrickBuffer;

/* Camera types supported by pins library.
 */
#define PINS_NO_CAMERA 0
#define PINS_TCD1304_CAMERA 1
#define PINS_WROVER_KIT_CAMERA 10
#define PINS_ESP_EYE_CAMERA 11
#define PINS_M5STACK_PSRAM_CAMERA 12
#define PINS_M5STACK_WIDE_CAMERA 13
#define PINS_AI_THINKER_CAMERA 14
#define PINS_USB_CAMERA 20
#define PINS_RASPI_CAMERA 30

/* If no camera defined for build, default to USB camera on windows,
 * Raspicam on Raspberry PI and to "no camera" on other platforms.
 */
#ifndef PINS_CAMERA
#if IOC_STREAMER_SUPPORT
  #ifdef OSAL_WINDOWS
    #define PINS_CAMERA PINS_USB_CAMERA
  #endif
  #ifdef OSAL_LINUX
    #ifdef E_OSVER_pi
      #define PINS_CAMERA PINS_RASPI_CAMERA
    #else
      #define PINS_CAMERA PINS_USB_CAMERA
    #endif
  #endif
#endif
#endif
#ifndef PINS_CAMERA
  #define PINS_CAMERA PINS_NO_CAMERA
#endif

/* Decide if we need inbuilt ESP32-CAMERA (included in /coderoot/pins/extensions/camera/esp32 directory),
   and cannot use one from library (as it should be used)
 */
#ifdef OSAL_ESP32
#ifdef OSAL_ESPIDF_FRAMEWORK
#if PINS_CAMERA!=PINS_NO_CAMERA
  #define PINS_INCLUDED_ESP_CAM_CODE_NEEDED
#endif
#endif
#endif

/* If we got a camera
 */
#if PINS_CAMERA

struct pinsCameraInterface;
struct pinsCameraInfo;
struct pinsCamera;
struct PinsCameraExt;

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

    /** Header structure, used to pass information for.
     */
    struct iocBrickHdr *hdr;

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

    /** Image format and compression. Compression 0 is uncompressed.
     */
    os_uchar format;
    os_uchar compression;
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
    /** Camera number, the first camera is camera number 0.
     */
    /* Moved to enumerated parameters
    os_int camera_nr; */

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

/** Camera resolution and color depth information.
 */
typedef struct pinsCameraResolution
{
    /** Image size, w = width in pixels, h = height in pixels.
     */
    os_ushort w, h;

    /** Image format and compression. OSAL_GRAYSCALE8, OSAL_GRAYSCALE16 or
         OSAL_RGB24.
     */
    os_uchar format;
}
pinsCameraResolution;

/** Maximum number of resolutions that can be returned for a camera
 */
#define PINS_CAMERA_MAX_RESOLUTIONS 16

/** Camera information structure.
 */
typedef struct pinsCameraInfo
{
    /** Camera number, the first camera is camera number 0.
     */
    os_short camera_nr;

    /** Number of resolutions in array below.
     */
    os_short nro_resolutions;

    /** Array of useful camera resolutions.
     */
    pinsCameraResolution resolution[PINS_CAMERA_MAX_RESOLUTIONS];

    /** Pointer to information about next camera.
     */
    struct pinsCameraInfo *next;
}
pinsCameraInfo;

/** Enumeration of camera parameters which can be modified.

    - PINS_CAM_INTEGRATION_US: Set integration time
    - PINS_CAM_INTEGRATION_US: Get maximum photo size in bytes including brick header

 */
typedef enum pinsCameraParamIx
{
    PINS_CAM_NR,                /* Camera number. 1 is first camera (0 = any camera) */
    PINS_CAM_IMG_WIDTH,         /* Image width, pixels */
    PINS_CAM_IMG_HEIGHT,        /* Image height, pixels */
    PINS_CAM_FRAMERATE,         /* Frame reate, pictures/second */

    PINS_CAM_BRIGHTNESS,
    PINS_CAM_SATURATION,

    PINS_CAM_INTEGRATION_US,

    PINS_NRO_CAMERA_PARAMS
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

    /** Camera number, the first camera is camera number 0.
     */
    os_int camera_nr;

    /** Camera parameters (should be obsoleted from here).
     */
    // os_long integration_us;

    /** Pointer to camera's pin structure.
     */
    const struct Pin *camera_pin;

    /** Camera timer pin. OS_NULL if not used by the wrapper.
     */
    const struct Pin *timer_pin;

    /** Camera wrapper specific extended camera data. OS_NULL if not used by the wrapper.
     */
    struct PinsCameraExt *ext;
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

    os_int (*enumerate_cameras)(
        pinsCameraInfo **camera_info);

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

    void (*set_parameter)(
        pinsCamera *c,
        pinsCameraParamIx ix,
        os_long x);

    os_long (*get_parameter)(
        pinsCamera *c,
        pinsCameraParamIx ix);

    void (*set_camera_jpeg_quality)(
        pinsCamera *c,
        os_uchar quality);

    void (*finalize_photo)(
        pinsPhoto *photo);
}
pinsCameraInterface;


/* Include operating system specific defines.
 */
#ifdef OSAL_LINUX
  #include "extensions/camera/linux/pins_linux_camera.h"
#endif

#ifdef OSAL_WINDOWS
  #include "extensions/camera/windows/pins_windows_camera.h"
#endif

#ifdef OSAL_ESP32
  #include "extensions/camera/esp32/pins_esp32_camera.h"
#endif

/* Store a photo as a "brick" within brick buffer for communication.
 */
osalStatus pins_store_photo_as_brick(
    const pinsPhoto *photo,
    struct iocBrickBuffer *b,
    os_uchar compression);

/* Release camera information chain.
 */
void pins_release_camera_info(
    pinsCameraInfo *camera_info);

#endif

#endif
