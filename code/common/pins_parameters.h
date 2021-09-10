/**

  @file    common/pins_parameters.h
  @brief   Run time access to IO pin parameters.
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
#ifndef PINS_PARAMETERS_H_
#define PINS_PARAMETERS_H_
#include "pins.h"

/* Modify IO pin parameter.
 */
void pin_set_prm(
    const Pin *pin,
    pinPrm prm,
    os_int value);

/* Get value of IO pin parmeter.
 */
os_int pin_get_prm(
    const Pin *pin,
    pinPrm prm);

/* Get fequency setting for the pin.
 */
os_int pin_get_frequency(
    const Pin *pin,
    os_int default_frequency);

/* Get speed setting for the pin.
 */
os_int pin_get_speed(
    const Pin *pin,
    os_int default_speed);

#endif
