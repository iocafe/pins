/**

  @file    common/pins_display.c
  @brief   Display hardware API.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    23.3.2020

  Code shared by different display hardwares to connect to eosal network state, etc.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "pins.h"
#if PINS_DISPLAY


/** Forward referred static functions.
 */
static void display_net_state_notification_handler(
    struct osalNetworkState *net_state,
    void *context);



/**
****************************************************************************************************

  @brief Setup a display.
  @anchor initialize_display

  The initialize_display() function initializes the display state structure.

  @param   display The display state structure.
  @param   prm Parameter structure.
  @param   root Pointer to iocom root object, OS_NULL if nome.
  @return  None.

****************************************************************************************************
*/
void initialize_display(
    PinsDisplay *display,
    const pinsDisplayParams *prm,
    iocRoot *root)
{
    os_memclear(display, sizeof(PinsDisplay));
    display->root = root;
    initialize_morse_code(&display->morse, OS_NULL, MORSE_DEFAULT);
    osal_add_network_state_notification_handler(display_net_state_notification_handler, display, 0);

    /* Call hardware/platform specific initialization code.
     */
    initialize_display_hw(display, prm);
}


/**
****************************************************************************************************

  @brief Set application specific text to display.
  @anchor set_display_text

  The set_display_text() function...

  @param   display The display state structure.
  @return  None.

****************************************************************************************************
*/
void set_display_text(
    PinsDisplay *display,
    os_char *text,
    os_short line,
    void *attr)
{
    // display->touched = OS_TRUE;
}


/**
****************************************************************************************************

  @brief Keep the display alive.
  @anchor run_display

  The run_display() function controls the display. This must be called repeatedly from ioboard
  loop.

  @param   display The display state structure.
  @param   timer Pointer to current timer value, or OS_NULL to get timer by this function.
  @return  None.

****************************************************************************************************
*/
void run_display(
    PinsDisplay *display,
    os_timer *timer)
{
    os_timer localtimer;
    os_boolean led_on;

    if (timer == OS_NULL)
    {
        os_get_timer(&localtimer);
        timer = &localtimer;
    }

    led_on = blink_morse_code(&display->morse, timer);
    if (led_on != display->state_led_on)
    {
        display->state_led_on = led_on;
        display->state_led_touched = OS_TRUE;
    }


    run_display_hw(display, timer);
}


/**
****************************************************************************************************

  @brief Handle network state change notifications.
  @anchor display_net_state_notification_handler

  The display_net_state_notification_handler() function is callback function when network state
  changes. Determines from network state if all is ok or something is wrong, and sets morse code
  accordingly.

  @param   net_state Network state structure.
  @param   context Morse code structure.
  @return  None.

****************************************************************************************************
*/
static void display_net_state_notification_handler(
    struct osalNetworkState *net_state,
    void *context)
{
    MorseCodeEnum code;
    PinsDisplay *display;
    display = (PinsDisplay*)context;

    code = network_state_to_morse_code(&display->morse, net_state);
    set_morse_code(&display->morse, code);
    display->code = code;
}

#endif
