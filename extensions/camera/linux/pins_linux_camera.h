/**

  @file    camera/linux/pins_linux_camera.h
  @brief   Linux specific camera defines.
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

#ifdef E_OSVER_pi
  extern const pinsCameraInterface pins_raspi_camera_iface;
  #define PINS_CAMERA_IFACE pins_raspi_camera_iface

  extern const pinsCameraInterface pins_usb_camera_iface;
  #define PINS_CAMERA_IFACE2 pins_usb_camera_iface
#else
  extern const pinsCameraInterface pins_usb_camera_iface;
  #define PINS_CAMERA_IFACE pins_usb_camera_iface
  #define PINS_CAMERA_IFACE2 pins_usb_camera_iface
#endif

extern const pinsCameraInterface pins_tcd1304_camera_iface;
#define PINS_LINEAR_CAMERA_IFACE pins_tcd1304_camera_iface

#endif
#endif
