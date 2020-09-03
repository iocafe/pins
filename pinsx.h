/**

  @file    pinsx.h
  @brief   Extended pins main header with IOCOM library support.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    21.4.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#pragma once
#ifndef PINSX_H_
#define PINSX_H_
#include "pins.h"

/* If C++ compilation, all functions, etc. from this point on in this header file are
   plain C and must be left undecorated.
 */
OSAL_C_HEADER_BEGINS

/* Include extenstion headers.
 */
#include "extensions/morse/common/pins_morse_code.h"
#include "extensions/devicebus/common/pins_devicebus.h"
#include "extensions/camera/common/pins_camera.h"
#include "extensions/detect_motion/common/pins_detect_motion.h"
#include "extensions/display/common/pins_display.h"
#include "extensions/iocom/common/pins_to_iocom.h"

/* If C++ compilation, end the undecorated code.
 */
OSAL_C_HEADER_ENDS

#endif
