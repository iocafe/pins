/**

  @file    extensions/morse/common/pins_morse_code.h
  @brief   Blink LED to display Morse code number.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    21.4.2020

  Reason why microcontroller is not connecting to network can be indicated by blinking a LED.
  Here we simply BLINK error number 1 - 9 as morse code. When there is no error (code is 0),
  we blink LED once per two seconds briefly.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#pragma once
#ifndef PINS_MORSE_CODE_H_
#define PINS_MORSE_CODE_H_
#include "pinsx.h"

#define NRO_MORSE_STEPS 10


/* Receipe how to bling the LED OFF and ON, contains ms timer values.
 */
typedef struct MorseRecipe
{
    os_short time_ms[NRO_MORSE_STEPS];
    os_int n;
}
MorseRecipe;

/* Morse code state structure.
 * Pin connected to the LED to blink.
   Pin connected to the second LED to blink. OS_NULL if no second LED.
 */
typedef struct MorseCode
{
    const Pin *pin;
    const Pin *pin2;
    os_timer timer;
    os_int code;
    os_int prev_code;
    os_int pos;
    os_boolean start_led_on;
    os_boolean led_on;

    os_short value_off[2];
    os_short value_blink_ok[2];
    os_short value_blink_attention[2];

    MorseRecipe recipe;
}
MorseCode;

/* Flags for initialize_morse_code()
 */
#define MORSE_DEFAULT 0
#define MORSE_LED_INVERTED 1
#define MORSE_HANDLE_NET_STATE_NOTIFICATIONS 2

/* Setup an LED output to blink by morse code.
 */
void initialize_morse_code(
    MorseCode *morse,
    const Pin *pin,
    const Pin *pin2,
    os_boolean flags);

/* Set morse code to indicate by blinking the led.
 */
void set_morse_code(
    struct MorseCode *morse,
    os_int code);

/* Keep the morse code LED alive.
 */
os_boolean blink_morse_code(
    struct MorseCode *morse,
    os_timer *timer);

/* Get text describing a morse code.
 */
const os_char *morse_code_to_text(
    osalMorseCodeEnum code);

#endif
