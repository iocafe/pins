/**

  @file    extensions/morse/common/pins_morse_code.c
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
#include "pinsx.h"


/** Forward referred static functions.
 */
static void morse_net_state_notification_handler(
    struct osalNetworkState *net_state,
    void *context);


/**
****************************************************************************************************

  @brief Setup an LED output to blink by morse code.
  @anchor initialize_morse_code

  The initialize_morse_code() function initializes MorseCode structure, the pin is stored pointer
  within the structure.

  @param   morse Morse code structure.
  @param   pin Pin connected to the LED to blink. Pointer to pin configuration structure.
  @param   Pin connected to the second LED to blink. OS_NULL if no second LED.
  @param   flags Zero for default operation, MORSE_LED_INVERTED to invert led output.

  @return  None.

****************************************************************************************************
*/
void initialize_morse_code(
    MorseCode *morse,
    const Pin *pin,
    const Pin *pin2,
    os_boolean flags)
{
    os_memclear(morse, sizeof(MorseCode));
    morse->pin = pin;
    morse->pin2 = pin2;
    morse->prev_code = -1;
    morse->start_led_on = (os_boolean)((flags & MORSE_LED_INVERTED) == 0);
    morse->blink_level[0] = morse->blink_level[1] = 1;
    morse->blink_attention_level[0] = morse->blink_attention_level[1] = 1;

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
    morse->code = code;
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
static void make_morse_recipe(
    struct MorseCode *morse)
{
    os_int i, n, code;

    code = morse->code;
    os_memclear(&morse->recipe, sizeof(MorseRecipe));
    n = 0;

    if (code == MORSE_PROGRAMMING_DEVICE)
    {
        morse->recipe.time_ms[n++] = 200;
        morse->recipe.time_ms[n++] = 200;
    }

    else if (code == MORSE_CONFIGURING)
    {
        morse->recipe.time_ms[n++] = 3000;
        morse->recipe.time_ms[n++] = 0;
    }

    else if (code == MORSE_CONFIGURATION_MATCH)
    {
        morse->recipe.time_ms[n++] = 1000;
        morse->recipe.time_ms[n++] = 200;
    }

    else if (code <= 0)
    {
        morse->recipe.time_ms[n++] = 100;
        morse->recipe.time_ms[n++] = 3000;
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
            morse->recipe.time_ms[n++] = 1200;
            morse->recipe.time_ms[n++] = 400;
        }
        morse->recipe.time_ms[n-1] = 5000;
    }
    else if (code <= 10)
    {
        for (i = 5; i < code; i++)
        {
            morse->recipe.time_ms[n++] = 1200;
            morse->recipe.time_ms[n++] = 400;
        }
        for (i = code; i < 10; i++)
        {
            morse->recipe.time_ms[n++] = 200;
            morse->recipe.time_ms[n++] = 300;
        }
        morse->recipe.time_ms[n-1] = 5000;
    }
    morse->recipe.n = n;

    osal_debug_assert(n <= NRO_MORSE_STEPS);
}


/**
****************************************************************************************************

  @brief Get value to write to IO ping matching to morse state.
  @anchor blink_get_pin_value

  The blink_get_pin_value() function gets value to set for output pin. Normally, for digital
  outputs, value 1 is used ofr LED on and value 0 for LED off. But it is possible to
  bling PWM output with different intensities, or to invert the signal using pin levels.
  This allows also blinking error coders with brighter intensity than normal operation.

  @param   morse Morse code structure.
  @param   pin_nr0 0 for first morse pin, 1 for second morse pin.
  @return  Value to write to pin

****************************************************************************************************
*/
static os_short blink_get_pin_value(
    struct MorseCode *morse,
    os_short pin_nr0)
{
    os_short value;

    if (morse->code == MORSE_RUNNING && morse->steady_hdlight_level[pin_nr0]) {
        return morse->steady_hdlight_level[pin_nr0];
    }

    if (morse->led_on)
    {
        if (morse->code == MORSE_RUNNING ||
            morse->code == MORSE_CONFIGURING ||
            morse->code == MORSE_NETWORK_NOT_CONNECTED)
        {
            value = morse->blink_level[pin_nr0];
        }
        else
        {
            value = morse->blink_attention_level[pin_nr0];
        }
    }
    else {
        value = morse->off_level[pin_nr0];
    }
    return value;
}


/**
****************************************************************************************************

  @brief Keep the morse code LED alive.
  @anchor blink_morse_code

  The blink_morse_code() function controls the LED. This must be called repeatedly from ioboard
  loop.

  @param   morse Morse code structure.
  @param   timer Pointer to current timer value, or OS_NULL to get timer by
           this function.
  @return  LED on or off.

****************************************************************************************************
*/
os_boolean blink_morse_code(
    struct MorseCode *morse,
    os_timer *timer)
{
    os_int pos;
    os_timer localtimer;

    if (morse->code != morse->prev_code)\
    {
        make_morse_recipe(morse);
        morse->prev_code = morse->code;
        morse->pos = 0;
        morse->led_on = morse->start_led_on;
        if (morse->pin) pin_set(morse->pin, blink_get_pin_value(morse, 0));
        if (morse->pin2) pin_set(morse->pin2, blink_get_pin_value(morse, 1));
    }

    if (timer == OS_NULL)
    {
        os_get_timer(&localtimer);
        timer = &localtimer;
    }

    pos = morse->pos;
    if (os_has_elapsed_since(&morse->timer, timer, morse->recipe.time_ms[pos]))
    {
        morse->led_on = !morse->led_on;

        if (morse->pin) pin_set(morse->pin, blink_get_pin_value(morse, 0));
        if (morse->pin2) pin_set(morse->pin2, blink_get_pin_value(morse, 1));
        morse->timer = *timer;
        if (++pos >= morse->recipe.n)
        {
            pos = 0;
        }
        morse->pos = pos;
    }

    return morse->led_on;
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
    osalMorseCodeEnum code;
    MorseCode *morse;
    morse = (MorseCode*)context;

    code = osal_network_state_to_morse_code(net_state);
    set_morse_code(morse, code);
}
