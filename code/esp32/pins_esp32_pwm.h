/**

  @file    esp32/pins_esp32_pwm.h
  @brief   ESP32 pulse width modulation.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    21.4.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/

/* Configure a pin as PWM.
 */
void pin_pwm_setup(
    const Pin *pin);
