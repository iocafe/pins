/**

  @file    pins.h
  @brief   Pins library main header file.
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
#ifndef PINS_H_
#define PINS_H_
#include "iocom.h"

/* If C++ compilation, all functions, etc. from this point on in this header file are
   plain C and must be left undecorated.
 */
OSAL_C_HEADER_BEGINS

/* PINS library version number.
 */
#ifndef PINS_VERSION
#define PINS_VERSION "210424"
#endif

/* Include target operating system specific defines.
 */
#ifndef PINS_TARGET_HDRS_INCLUDED
#ifdef E_OS_esp32
  #include "code/esp32/pins_hw_defs.h"
  #define PINS_TARGET_HDRS_INCLUDED
#endif
#endif

#ifndef PINS_TARGET_HDRS_INCLUDED
#ifdef E_OS_arduino
  #include "code/duino/pins_hw_defs.h"
  #define PINS_TARGET_HDRS_INCLUDED
#endif
#endif

#ifndef PINS_TARGET_HDRS_INCLUDED
#ifdef E_OS_linux
  #ifdef E_OSVER_pi
    #include "code/pigpio/pins_hw_defs.h"
  #else
    #include "code/simulation/pins_hw_defs.h"
  #endif
  #define PINS_TARGET_HDRS_INCLUDED
#endif
#endif

#ifndef PINS_TARGET_HDRS_INCLUDED
#ifdef E_OS_windows
  #include "code/simulation/pins_hw_defs.h"
  #define PINS_TARGET_HDRS_INCLUDED
#endif
#endif

#ifndef PINS_TARGET_HDRS_INCLUDED
#ifdef E_OS_metal
  #include "code/metal/pins_hw_defs.h"
  #define PINS_TARGET_HDRS_INCLUDED
#endif
#endif

/* Defaults for SPI and I2C support, if none specified in OS specific headers
   or for the build.
 */
#ifndef PINS_SPI
  #define PINS_SPI 1
#endif
#ifndef PINS_I2C
  #define PINS_I2C 1
#endif

/* Include generic pins library headers.
 */
#include "code/common/pins_gpio.h"
#include "code/common/pins_timer.h"
#include "code/common/pins_basics.h"
#include "code/common/pins_state.h"
#include "code/common/pins_parameters.h"

/* If C++ compilation, end the undecorated code.
 */
OSAL_C_HEADER_ENDS

#endif
