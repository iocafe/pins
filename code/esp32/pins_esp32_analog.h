/**

  @file    esp32/pins_esp32_analog.h
  @brief   ADC & DAC.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    26.4.2021

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/

/* Configure a pin as analog input.
 */
void pin_setup_analog_input(
    const Pin *pin);

/* Read analog input.
 */
os_int pin_read_analog_input(
    const Pin *pin,
    os_char *state_bits);

/* Configure a pin as analog output.
 */
void pin_setup_analog_output(
    const Pin *pin);

/* Set analog output.
 */
void pin_write_analog_output(
    const Pin *pin,
    os_int x);
