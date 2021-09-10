/**

  @file    esp32/pins_hw_defs.h
  @brief   Operating system specific defines for the pins library.
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
#ifndef PINS_HW_DEFS_H_
#define PINS_HW_DEFS_H_
#include "pins.h"

typedef void pin_interrupt_handler(void);

/* Arduino defaults
 */
#ifndef PINS_SPI
    #define PINS_SPI 0
#endif

#ifndef PINS_I2C
    #define PINS_I2C 0
#endif

#ifndef PINS_SIMULATION
    #define PINS_SIMULATION 0
#endif

#ifndef PINS_SIMULATED_INTERRUPTS
    #define PINS_SIMULATED_INTERRUPTS 0
#endif

#endif
