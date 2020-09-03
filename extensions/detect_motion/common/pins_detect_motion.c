/**

  @file    extensions/detect_motion/common/pins_detect_motion.c
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
#include "pinsx.h"
#if PINS_CAMERA

/**
****************************************************************************************************

  @brief X.
  @anchor initialize_detect_motion_code

  The initialize_detect_motion_code() function initializes motion detection object structure.

  @param   detect_motion Motion detection object structure.
  @return  None.

****************************************************************************************************
*/
void initialize_motion_detection(
    DetectMotion *detect_motion)
{
}

void release_motion_detection(
    DetectMotion *detect_motion)
{
}

osalStatus detect_motion(
    DetectMotion *detect_motion,
    const struct pinsPhoto *photo,
    MotionDetectionParameters *prm,
    MotionDetectionResults *res)
{
    return OSAL_SUCCESS;
}

void set_motion_detection_image(
    DetectMotion *detect_motion,
    const struct pinsPhoto *photo)
{
}

#endif
