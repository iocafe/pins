/**

  @file    arduino/pins_display_tft_espi.c
  @brief   SPI TFT display wrapper for ESP/Arduino platform.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    23.3.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "pins.h"
#if PINS_DISPLAY == PINS_TFT_ESPI
#include "Arduino.h"

#include <SPI.h>
#include <TFT_eSPI.h>       // Hardware-specific library


TFT_eSPI tft = TFT_eSPI();  // Invoke custom library



/**
****************************************************************************************************

  @brief Hardware specific display setup (called by initialize_display())
  @anchor initialize_display_hw

  The initialize_display_hw() function initializes display hardware.

  @param   display The display state structure.
  @param   reserved Reserved for future.
  @return  None.

****************************************************************************************************
*/
void initialize_display_hw(
    PinsDisplay *display,
    void *reserved)
{
    tft.init();

    tft.fillScreen(TFT_BLACK);

    // Set "cursor" at top left corner of display (0,0) and select font 4
    tft.setCursor(0, 0, 4);

    // Set the font colour to be white with a black background
    tft.setTextColor(TFT_WHITE, TFT_BLACK);

    // We can now plot text on screen using the "print" class
    tft.println("Intialised default\n");
    tft.println("White text");

    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.println("Red text");

    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.println("Green text");

    tft.setTextColor(TFT_BLUE, TFT_BLACK);
    tft.println("Blue text");

osal_debug_error("HERE SSSSSSSSSSSSSSSSSSSSSSSX")          ;
}


/**
****************************************************************************************************

  @brief Keep the display alive.
  @anchor run_display

  The run_display() function controls the display. This must be called repeatedly from ioboard
  loop.

  @param   display The display state structure.
  @param   timer Pointer to current timer value.
  @return  None.

****************************************************************************************************
*/
void run_display_hw(
    PinsDisplay *display,
    os_timer *timer)
{
    if (!display->state_led_touched) return;
    display->state_led_touched = OS_FALSE;

    if (display->state_led_on)
    {
          tft.invertDisplay( false ); // Where i is true or false

          tft.fillScreen(TFT_BLACK);

          tft.setCursor(0, 0, 4);

          tft.setTextColor(TFT_WHITE, TFT_BLACK);
          tft.println("Invert OFF\n");

          tft.println("Kanootti");

          tft.setTextColor(TFT_RED, TFT_BLACK);
          tft.println("Urhea");

          tft.setTextColor(TFT_GREEN, TFT_BLACK);
          tft.println("Saapas");

          tft.setTextColor(TFT_BLUE, TFT_BLACK);
          tft.println("Blue text");

osal_debug_error("HERE X")          ;
    }
    else
    {
        // Binary inversion of colours
        tft.invertDisplay( true ); // Where i is true or false

        tft.fillScreen(TFT_BLACK);

        tft.setCursor(0, 0, 4);

        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.println("Invert ON\n");

        tft.println("Eika");

        tft.setTextColor(TFT_RED, TFT_BLACK);
        tft.println("ikina");

        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        tft.println("uppoa");

        tft.setTextColor(TFT_BLUE, TFT_BLACK);
        tft.println("Blue text");
    }
}

#endif
