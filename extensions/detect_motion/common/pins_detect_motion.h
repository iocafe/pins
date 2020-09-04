/**

  @file    extensions/detect_motion/common/pins_detect_motion.h
  @brief   Pins library extension for motion detection from camera image.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    3.9.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#pragma once
#ifndef PINS_DETECT_MOTION_H_
#define PINS_DETECT_MOTION_H_
#include "pinsx.h"
#if PINS_CAMERA

struct pinsPhoto;

/* Parameters for motion detection.
 */
typedef struct MotionDetectionParameters
{
    /* Timer reading when motion detection is called. If zero, os_gettimer is called.
     */
    os_timer ti;

    /* Maximum and minimum frame interval (1/frame rate).
     */
    os_int min_interval_ms;
    os_int max_interval_ms;

    /* Limit of movement. Roughly change of pixel's gray level squared (more complex in real)
     */
    os_int movement_limit;
}
MotionDetectionParameters;


/* Motion detection results as numbers.
 */
typedef struct MotionDetectionResults
{
    int x;
}
MotionDetectionResults;


/* DetectMotion code state structure.
 * Pin connected to the LED to blink.
   Pin connected to the second LED to blink. OS_NULL if no second LED.
 */
typedef struct DetectMotion
{
    /* Half image (1/4 size) image witdth and height in pixlels.
     */
    os_int h_w, h_h;

    /* Quarter image (1/4 size) image witdth and height in pixlels.
     */
    os_int q_w, q_h;

    /* Half size image buffers
     */
    os_uchar *h_buf1, *h_buf2;

    /* Allocated size for half image buffers.
     */
    os_memsz h_buf1_alloc, h_buf2_alloc;

    /* Quarter image  (1/16 size) buffers
     */
    os_uchar *q_new, *q_prev;

    /* Allocated size for quarter image buffers.
     */
    os_memsz q_new_alloc, q_prev_alloc;

    /* Total sums of gray values for image.
     */
    os_ulong q_new_sum, q_prev_sum;

    /* Timer reading when last image was set
     */
    os_timer image_set_ti;
}
DetectMotion;


void initialize_motion_detection(
    DetectMotion *dm);

void release_motion_detection(
    DetectMotion *dm);

osalStatus detect_motion(
    DetectMotion *dm,
    const struct pinsPhoto *photo,
    MotionDetectionParameters *prm,
    MotionDetectionResults *res);

void set_motion_detection_image(
    DetectMotion *dm,
    const struct pinsPhoto *photo);

#endif
#endif
