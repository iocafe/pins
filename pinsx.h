/**

  @file    pinsx.h
  @brief   Extended pins main header with IOCOM library support.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    8.1.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#ifndef PINSX_INCLUDED
#define PINSX_INCLUDED

/* Pins basics and operating system abstraction layer headers.
 */
#include "pins.h"

/* If C++ compilation, all functions, etc. from this point on in this header file are
   plain C and must be left undecorated.
 */
OSAL_C_HEADER_BEGINS

/* Include IOCOM support headers.
 */
#include "code/common/pins_to_iocom.h"

/* If C++ compilation, end the undecorated code.
 */
OSAL_C_HEADER_ENDS

#endif
