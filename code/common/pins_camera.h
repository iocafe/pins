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
#define PINS_CAMERA PINS_NO_CAMERA
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

    /** Image data buffer and  buffer size in bytes.
     */
    os_uchar *buf;
    os_memsz buf_sz;

    /** Image width in bytes, pointer to pixel below p[0] is p[byte_w].
     */
    os_int byte_w;

    /** Image size, w = width in pixels, h = height in pixels.
     */
    os_int w, h;
}
pinsCameraImage;


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
    const struct pinsCameraInterface *iface;

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
    PINS_CAM_INTEGRATION_NS,
    PINS_CAM_FLAGH_NS
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
    osalThreadHandle *camera_thread;

    /** Event to trigger camera thread, OS_NULL if none.
     */
    osalEvent camera_event;

    /** Flag to stop camera thread.
     */
    volatile os_boolean stop_thread;

    /** Camera identifier, used internally by implementation.
     */
    os_int id;

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

	void (*release_image)(
        pinsCameraImage *image);
}
pinsCameraInterface;


void pins_release_camera_image(
    pinsCameraImage *image);


#if PINS_CAMERA == PINS_TDC1304_CAMERA
  extern const pinsCameraInterface pins_tdc1304_camera_iface;
  #define PINS_CAMERA_IFACE pins_tdc1304_camera_iface
#endif

#endif
