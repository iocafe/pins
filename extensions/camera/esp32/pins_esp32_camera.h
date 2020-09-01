/**

  @file    extensions/camera/esp32/pins_esp32_camera.h
  @brief   ESP32 specific camera defines.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    30.8.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#pragma once
#ifndef PINS_LINUX_CAMERA_H_
#define PINS_LINUX_CAMERA_H_
#include "pinsx.h"

#if PINS_CAMERA

#if PINS_CAMERA == PINS_WROVER_KIT_CAMERA || \
    PINS_CAMERA == PINS_ESP_EYE_CAMERA || \
    PINS_CAMERA == PINS_M5STACK_PSRAM_CAMERA || \
    PINS_CAMERA == PINS_M5STACK_WIDE_CAMERA || \
    PINS_CAMERA == PINS_AI_THINKER_CAMERA
  #define PINS_IS_ESP32_CAMERA 1
#else
  #define PINS_IS_ESP32_CAMERA 0
#endif

#if PINS_IS_ESP32_CAMERA
  extern const pinsCameraInterface pins_esp32_camera_iface;
  #define PINS_CAMERA_IFACE pins_esp32_camera_iface
#endif

#if PINS_CAMERA == PINS_TCD1304_CAMERA
  extern const pinsCameraInterface pins_tcd1304_camera_iface;
  #define PINS_LINEAR_CAMERA_IFACE pins_tcd1304_camera_iface
#endif

#endif
#endif
