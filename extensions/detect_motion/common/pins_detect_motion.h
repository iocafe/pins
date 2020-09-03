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
    int x;

    // Autoset if above motion x;
}
MotionDetectionParameters;


/* Motion detection results as numbers.
 */
typedef struct MotionDetectionResults
{
    int x;

    // Autoset if above motion x;
}
MotionDetectionResults;


/* DetectMotion code state structure.
 * Pin connected to the LED to blink.
   Pin connected to the second LED to blink. OS_NULL if no second LED.
 */
typedef struct DetectMotion
{
    int x;
}
DetectMotion;

void initialize_motion_detection(
    DetectMotion *detect_motion);

void release_motion_detection(
    DetectMotion *detect_motion);

osalStatus detect_motion(
    DetectMotion *detect_motion,
    const struct pinsPhoto *photo,
    MotionDetectionParameters *prm,
    MotionDetectionResults *res);

void set_motion_detection_image(
    DetectMotion *detect_motion,
    const struct pinsPhoto *photo);

#endif
#endif
