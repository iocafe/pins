/**

  @file    common/pins_morse_code.c
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
#include "pins.h"


/** Forward referred static functions.
 */
static void morse_net_state_notification_handler(
    struct osalNetworkState *net_state,
    void *context);


/**
****************************************************************************************************

  @brief Setup an LED output to blink by more code.
  @anchor morse_code_setup

  The morse_code_setup() function initializes MorseCode structure, the pin is stored pointer
  within the structure.

  @param   morse Morse code structure.
  @param   pin Pointer to pin configuration structure.
  @param   flags Zero for default operation, MORSE_LED_INVERTED to invert led output.

  @return  None.

****************************************************************************************************
*/
void morse_code_setup(
    MorseCode *morse,
    const Pin *pin,
    os_boolean flags)
{
    os_memclear(morse, sizeof(MorseCode));
    morse->pin = pin;
    morse->code = -1;
    morse->led_on = (os_boolean)((flags & MORSE_LED_INVERTED) == 0);
    set_morse_code(morse, 0);

    if (flags & MORSE_HANDLE_NET_STATE_NOTIFICATIONS)
    {
        osal_add_network_state_notification_handler(morse_net_state_notification_handler, morse, 0);
    }
}


/**
****************************************************************************************************

  @brief Set morse code to indicate by blinking the led.
  @anchor set_morse_code

  The set_morse_code() function stores the error code to indicate within the MorseCode structure.

  Receipe is pair number of OFF, ON times

  @param   morse Morse code structure.
  @param   code Error number 1 - 9 to blink or 0 if all is fine.
  @return  None.

****************************************************************************************************
*/
void set_morse_code(
    struct MorseCode *morse,
    os_int code)
{
    os_int i, n;

    if (code == morse->code) return;
    morse->code = code;

    os_lock();
    os_memclear(&morse->recipe, sizeof(MorseRecipe));
    n = 0;

    if (code <= 0)
    {
        morse->recipe.time_ms[n++] = 100;
        morse->recipe.time_ms[n++] = 2000;
    }

    else if (code <= 5)
    {
        for (i = 0; i < code; i++)
        {
            morse->recipe.time_ms[n++] = 200;
            morse->recipe.time_ms[n++] = 300;
        }
        morse->recipe.time_ms[n-1] = 600;
        for (i = code; i < 5; i++)
        {
            morse->recipe.time_ms[n++] = 800;
            morse->recipe.time_ms[n++] = 400;
        }
        morse->recipe.time_ms[n-1] = 3000;
    }
    else if (code <= 10)
    {
        for (i = 5; i < code; i++)
        {
            morse->recipe.time_ms[n++] = 800;
            morse->recipe.time_ms[n++] = 400;
        }
        for (i = code; i < 10; i++)
        {
            morse->recipe.time_ms[n++] = 200;
            morse->recipe.time_ms[n++] = 300;
        }
        morse->recipe.time_ms[n-1] = 3000;
    }
    morse->recipe.n = n;
    os_unlock();

    osal_debug_assert(n <= NRO_MORSE_STEPS);
}


/**
****************************************************************************************************

  @brief Keep the morse code LED alive.
  @anchor blink_morse_code

  The blink_morse_code() function controls the LED. This must be called repeatedly from ioboard
  loop.

  @param   morse Morse code structure.
  @param   timer Pointer to timer to a save function call, or OS_NULL to get timer by
           this function.
  @return  None.

****************************************************************************************************
*/
void blink_morse_code(
    struct MorseCode *morse,
    os_timer *timer)
{
    os_int pos;
    os_timer localtimer;

    if (timer == OS_NULL)
    {
        os_get_timer(&localtimer);
        timer = &localtimer;
    }

    pos = morse->pos;
    if (os_elapsed2(&morse->timer, timer, morse->running.time_ms[pos]))
    {
        morse->led_on = !morse->led_on;
        pin_set(morse->pin, morse->led_on);
        morse->timer = *timer;
        if (++pos >= morse->running.n)
        {
            os_lock();
            morse->running = morse->recipe;
            os_unlock();
            pos = 0;
        }
        morse->pos = pos;
    }
}


/**
****************************************************************************************************

  @brief Handle network state change notifications.
  @anchor morse_net_state_notification_handler

  The morse_net_state_notification_handler() function is callback function when network state
  changes. Determines from network state if all is ok or something is wrong, and sets morse code
  accordingly.

  @param   net_state Network state structure.
  @param   context Morse code structure.
  @return  None.

****************************************************************************************************
*/
static void morse_net_state_notification_handler(
    struct osalNetworkState *net_state,
    void *context)
{
    NetworkStateMorseCode code;
    MorseCode *morse;
    morse = (MorseCode*)context;

    /* If we do not have widi
     */
    if (net_state->wifi_used && !net_state->wifi_connected)
    {
        code = MORSE_NO_WIFI;
        goto setit;
    }

    /* All running fine.
     */
    code = MORSE_RUNNING;

setit:
    /* Set morse code to indicate network state.
     */
    set_morse_code(morse, code);
}