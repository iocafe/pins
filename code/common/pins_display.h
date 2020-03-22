/**

  @file    common/pins_display.h
  @brief   Display hardware API.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    23.3.2020

  This header file function prototypes, defines and types to interface application with a
  specific display hardware wrapper. The application calls these functions and the
  hardware wrapper implements them. Again, the goal is to allow switching hardware without
  changing the application.

  Here we assume that the display is used to display device state, any possible errors
  and when the device is working, what it does. This error information is received from
  eosal network state by state change notifications.

  Application can also call the display wrapper directly to display what device
  is currently working on.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
/* Pins display types
 */
#define PINS_NO_DISPLAY 0
#define PINS_TFT_ESPI 1

/* If no display defined for build, set 0
 */
#ifndef PINS_DISPLAY
#define PINS_DISPLAY PINS_NO_DISPLAY
#endif

/* If we got a display.
 */
#if PINS_DISPLAY

/* Number of text lines and line width in characters.
 */
#define PINS_DISPLAY_ROWS 3
#define PINS_DISPLAY_COLUMNS 30

/* Display state structure.
 */
typedef struct PinsDisplay
{
    /* Display shows also morse code indicators.
     */
    MorseCode morse;
    MorseCodeEnum code;

    iocRoot *root;

    /* Text content set by application.
     */
    os_char text[PINS_DISPLAY_ROWS][PINS_DISPLAY_COLUMNS];

    os_boolean
        state_led_on,
        state_led_touched,
        title_touched,
        show_network_name;

    os_timer
        title_timer;
}
PinsDisplay;

/* Setup a display.
 */
void initialize_display(
    PinsDisplay *display,
    iocRoot *root,
    void *reserved);

/* Set text or picture to display.
 */
void set_display_text(
    PinsDisplay *display,
    os_char *text,
    os_short line,
    void *attr);

/* Keep the display alive.
 */
void run_display(
    PinsDisplay *display,
    os_timer *timer);

/* The initialize_display_hw() function initializes display hardware (called by
   initialize_display function).
 */
void initialize_display_hw(
    PinsDisplay *display,
    void *reserved);

/* Run display hardware (called by run_display)
 */
void run_display_hw(
    PinsDisplay *display,
    os_timer *timer);

#endif
