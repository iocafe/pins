/**

  @file    common/pins_gpio.h
  @brief   Interrups and handlers.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    21.4.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/

struct Pin;



typedef struct SpiDevice
{
    /** Pointer to interrupt handler function.
     */
    pin_interrupt_handler *int_handler_func;

    /** Timer if we are simulating timer interrupts.
     */
    os_timer hit_timer;

    /** Interrupt mode flags.
     */
    struct SpiDevice *next;
}
SpiDevice;


/* Attach and interrupt to a GPIO pin.
 */
void pins_add_spi_device(
    const struct Pin *miso,
    const struct Pin *mosi,
    const struct Pin *clock,
    const struct Pin *cs,
    pinInterruptParams *prm);

/* Detach interrupt from GPIO pin.
 */
void pin_gpio_detach_interrupt(
    const struct Pin *pin);

#if PINS_SIMULATED_INTERRUPTS

/* Trigger a simulalated interrupt if flags match to x change.
 */
void pin_gpio_simulate_interrupt(
    const struct Pin *pin,
    os_int x);

#endif
