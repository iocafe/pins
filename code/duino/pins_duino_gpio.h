/**

  @file    duino/pins_duino_gpio.h
  @brief   GPIO pins.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    21.4.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/

/* Setup a pin as input.
 */
void pin_gpio_setup_input(
    const Pin *pin);

/* Setup a pin as output.
 */
void pin_gpio_setup_output(
    const Pin *pin);

/* Attach an interrupt to a GPIO pin.
 */
void pin_gpio_attach_interrupt(
    const struct Pin *pin,
    pinInterruptParams *prm);

/* Detach interrupt from GPIO pin.
 */
void pin_gpio_detach_interrupt(
    const struct Pin *pin);

