/**

  @file    common/pins_morse_code.h
  @brief   Blink LED to display Morse code number.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    2.3.2020

  Reason why microcontroller is not connecting to network can be indicated by blinking a LED.
  Here we simply BLINK error number 1 - 9 as morse code. When there is no error (code is 0),
  we blink LED once per two seconds briefly.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/

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
 */
typedef struct MorseCode
{
    const Pin *pin;
    os_timer timer;
    os_int code;
    os_int pos;
    os_boolean led_on;

    MorseRecipe recipe;
    MorseRecipe running;
}
MorseCode;

/* Flags for morse_code_setup()
 */
#define MORSE_LED_INVERTED 1


/* Setup an LED output to blink by more code.
 */
void morse_code_setup(
    MorseCode *morse,
    const Pin *pin,
    os_boolean flags);

/* Set morse code to indicate by blinking the led.
 */
void set_morse_code(
    struct MorseCode *morse,
    os_int code);

/* Keep the morse code LED alive.
 */
void blink_morse_code(
    struct MorseCode *morse,
    os_timer *timer);
