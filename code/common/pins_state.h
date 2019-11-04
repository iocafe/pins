/**

  @file    common/pins_state.h
  @brief   Set and get pin states.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    3.11.2019

  Copyright 2012 - 2019 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#ifndef PINS_STATE
#define PINS_STATE

/* Setup IO hardware for a device.
 */
void pins_setup(
    const IoPinsHdr *pins_hdr,
    os_int flags);

/* Set IO pin state.
 */
void pin_set(
    const Pin *pin,
    os_int state);

/* Get pin state.
 */
os_int pin_get(
    const Pin *pin);

/* Get pin state without reading hardware.
 */
os_int pin_value(
    const Pin *pin);

/* Read all inputs of the IO device into global Pin structurees
 */
void pins_read_all(
    const IoPinsHdr *hdr);


#endif
