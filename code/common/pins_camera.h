/**

  @file    common/pins_camera.h
  @brief   Camera hardware API.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    30.3.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/

/* Camera types supported by pins libeary
 */
#define PINS_NO_CAMERA 0
#define PINS_TDC1304_CAMERA 1

/* If no camera defined for build, set 0
 */
#ifndef PINS_CAMERA
// #define PINS_CAMERA PINS_NO_CAMERA
#define PINS_CAMERA PINS_TDC1304_CAMERA
#endif

/* If we got a camera
 */
#if PINS_CAMERA

struct pinsCameraInterface;


/** Camera image as received by camera callback function.
 */
typedef struct pinsCameraImage
{
    /** Camera interface (structure of camera implementation function pointers).
     */
    const struct pinsCameraInterface *iface;

    /** Image buffer and buffer size in bytes. Image buffer starts with
        flat pinsCameraImageBufHdr folloed by image pixel data.
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
pinsCameraImage;


/** Camera image as received by camera callback function.
    This must be flat.
    format Stream data format
 */
#define PINS_CAMERA_IMG_TSTAMP_SZ 8
typedef struct pinsCameraImageBufHdr
{
    os_uchar format;
    os_uchar reserved;
    os_uchar checksum_low;
    os_uchar checksum_high;
    os_uchar width_low;
    os_uchar width_high;
    os_uchar height_low;
    os_uchar height_high;
    os_uchar tstamp[PINS_CAMERA_IMG_TSTAMP_SZ];
}
pinsCameraImageBufHdr;


/** Camera callback function type, called when a frame has been captured.
 */
typedef void pinsCameraCallbackFunc(
    pinsCameraImage *image,
    void *context);


/** Parameters for open() function.
 */
typedef struct pinsCameraParams
{
    /** Camera interface (structure of camera implementation function pointers).
     */
    // const struct pinsCameraInterface *iface;

    /** Pointer to callback function and application specific content pointer to pass
        to the callback function..
     */
    pinsCameraCallbackFunc *callback_func;
    void *callback_context;

#if PINS_CAMERA == PINS_TDC1304_CAMERA
    const struct Pin *camera_pin;
    const struct Pin *timer_pin;
#endif
}
pinsCameraParams;


/** Enumeration of camera parameters which can be modified.
 */
typedef enum pinsCameraParamIx
{
    PINS_CAM_INTEGRATION_US
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

#if PINS_CAMERA == PINS_TDC1304_CAMERA
    const struct Pin *camera_pin;
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
}
pinsCameraInterface;

/* Store check sum within image
 */
void pins_set_camera_image_checksum(
    pinsCameraImage *image);

/* Store time stamp within image (must be called before pins_set_camera_image_checksum)
 */
void pins_set_camera_image_timestamp(
    pinsCameraImage *image);

#if PINS_CAMERA == PINS_TDC1304_CAMERA
  extern const pinsCameraInterface pins_tdc1304_camera_iface;
  #define PINS_CAMERA_IFACE pins_tdc1304_camera_iface
#endif

#endif
