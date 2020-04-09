/**

  @file    pins.h
  @brief   Pins library main header file.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    8.1.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#ifndef PINS_INCLUDED
#define PINS_INCLUDED

/* Include operating system abstraction layer.
 */
#include "iocom.h"

/* If C++ compilation, all functions, etc. from this point on in this header file are
   plain C and must be left undecorated.
 */
OSAL_C_HEADER_BEGINS

/* Include target operating system specific defines.
 */
#ifndef PINS_TARGET_HDRS_INCLUDED
#ifdef E_OS_arduino
  #include "code/esp32/pins_hw_defs.h"
  #define PINS_TARGET_HDRS_INCLUDED
#endif
#endif

#ifndef PINS_TARGET_HDRS_INCLUDED
#ifdef E_OS_linux
  #include "code/simulation/pins_hw_defs.h"
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


/* Include generic pins library headers.
 */
#include "code/common/pins_gpio.h"
#include "code/common/pins_timer.h"
#include "code/common/pins_basics.h"
#include "code/common/pins_state.h"
#include "code/common/pins_parameters.h"
#include "code/common/pins_morse_code.h"
#include "code/common/pins_display.h"
#include "code/common/pins_camera.h"

/* If C++ compilation, end the undecorated code.
 */
OSAL_C_HEADER_ENDS

#endif
