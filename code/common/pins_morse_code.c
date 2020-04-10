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

  @brief Setup an LED output to blink by morse code.
  @anchor initialize_morse_code

  The initialize_morse_code() function initializes MorseCode structure, the pin is stored pointer
  within the structure.

  @param   morse Morse code structure.
  @param   pin Pointer to pin configuration structure.
  @param   flags Zero for default operation, MORSE_LED_INVERTED to invert led output.

  @return  None.

****************************************************************************************************
*/
void initialize_morse_code(
    MorseCode *morse,
    const Pin *pin,
    os_boolean flags)
{
    os_memclear(morse, sizeof(MorseCode));
    morse->pin = pin;
    morse->prev_code = -1;
    morse->start_led_on = (os_boolean)((flags & MORSE_LED_INVERTED) == 0);

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

    if (code == MORSE_CONFIGURING)
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

    osal_debug_assert(n <= NRO_MORSE_STEPS);
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
        if (morse->pin) pin_set(morse->pin, morse->led_on);
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
        if (morse->pin) pin_set(morse->pin, morse->led_on);
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

  @brief Get morse code corresponding to network state.
  @anchor network_state_to_morse_code

  The network_state_to_morse_code() function examines network state structure and selects which
  morse code best describes it.

  @param   morse Morse code structure.
  @param   net_state Network state structure.
  @return  Morse code enumeration value.

****************************************************************************************************
*/
MorseCodeEnum network_state_to_morse_code(
    struct MorseCode *morse,
    struct osalNetworkState *net_state)
{
    MorseCodeEnum code;
    osaLightHouseClientState lighthouse_state;
    osalGazerbeamConnectionState gbs;

    /* If Gazerbeam configuration (WiFi with Android phone) is on?
     */
    gbs = osal_get_network_state_int(OSAL_NS_GAZERBEAM_CONNECTED, 0);
    if (gbs)
    {
        code = (gbs == OSAL_NS_GAZERBEAM_CONFIGURATION_MATCH)
                ? MORSE_CONFIGURATION_MATCH : MORSE_CONFIGURING;
        goto setit;
    }

    /* If WiFi is not connected?
     */
    if (osal_get_network_state_int(OSAL_NS_NETWORK_USED, 0) &&
        !osal_get_network_state_int(OSAL_NS_NETWORK_CONNECTED, 0))
    {
        code = MORSE_NETWORK_NOT_CONNECTED;
        goto setit;
    }

    /* Check for light house.
     */
    lighthouse_state
        = (osaLightHouseClientState)osal_get_network_state_int(OSAL_NS_LIGHTHOUSE_STATE, 0);
    if (lighthouse_state != OSAL_LIGHTHOUSE_NOT_USED &&
        lighthouse_state != OSAL_LIGHTHOUSE_OK)
    {
        code = (lighthouse_state == OSAL_LIGHTHOUSE_NOT_VISIBLE)
            ? MORSE_LIGHTHOUSE_NOT_VISIBLE : MORSE_NO_LIGHTHOUSE_FOR_THIS_IO_NETWORK;
        goto setit;
    }

    /* Certificates/keys not loaded.
     */
    if (/* osal_get_network_state_int(OSAL_NS_SECURITY_CONF_ERROR, 0) || */
        osal_get_network_state_int(OSAL_NS_NO_CERT_CHAIN, 0))
    {
        code = MORSE_SECURITY_CONF_ERROR;
        goto setit;
    }

    /* If no connected sockets?
     */
    if (osal_get_network_state_int(OSAL_NRO_CONNECTED_SOCKETS, 0) == 0)
    {
        code = MORSE_NO_CONNECTED_SOCKETS;
        goto setit;
    }

    /* All running fine.
     */
    code = MORSE_RUNNING;

setit:
    return code;
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
    MorseCodeEnum code;
    MorseCode *morse;
    morse = (MorseCode*)context;

    code = network_state_to_morse_code(morse, net_state);
    set_morse_code(morse, code);
}
