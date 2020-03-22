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
    display->title_touched = OS_TRUE;
    os_get_timer(&display->title_timer);
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
    uint16_t title_bgr_color, color;
    os_char buf[IOC_NETWORK_NAME_SZ], nbuf[OSAL_NBUF_SZ];

#define DISPLAY_W 240
#define DISPLAY_H 240
#define DISPLAY_TITLE_H 34
#define DISPLAY_SEPARATOR_H 3
#define DISPLAY_LED_SZ 16
#define DISPLAY_LED_SPC ((DISPLAY_TITLE_H - DISPLAY_LED_SZ) / 2)

    if (os_has_elapsed_since(&display->title_timer, timer, 3000))
    {
        display->title_touched = OS_TRUE;
        display->show_network_name = !display->show_network_name;
        display->title_timer = *timer;
    }

    /* Draw title background.
     */
    if (display->title_touched)
    {
        title_bgr_color = TFT_BLACK;
        tft.fillRect(0, 0, DISPLAY_W, DISPLAY_TITLE_H, title_bgr_color);
        tft.fillRect(0, DISPLAY_TITLE_H, DISPLAY_W, DISPLAY_SEPARATOR_H, TFT_PURPLE);

        if (display->root)
        {
            if (display->show_network_name)
            {
                os_strncpy(buf, display->root->network_name, sizeof(buf));
            }
            else
            {
                os_strncpy(buf, display->root->device_name, sizeof(buf));
                if (display->root->device_nr && display->root->device_nr != IOC_AUTO_DEVICE_NR)
                {
                    osal_int_to_str(nbuf, sizeof(nbuf), display->root->device_nr);
                    os_strncat(buf, nbuf, sizeof(buf));
                }
            }

            tft.setTextDatum(CC_DATUM);
            // tft.setCursor(DISPLAY_LED_SZ + DISPLAY_LED_SPC * 2, 8, 4);
            tft.setTextColor(TFT_WHITE, title_bgr_color);
            tft.drawString(buf, DISPLAY_W / 2, DISPLAY_TITLE_H / 2, 4);
        }

        display->state_led_touched = OS_TRUE;
        display->title_touched = OS_FALSE;
    }

    if (display->state_led_touched)
    {
        color = title_bgr_color;
        if (display->state_led_on) {
            switch (display->code)
            {
                case MORSE_CONFIGURING: color = TFT_YELLOW; break;
                case MORSE_RUNNING: color = TFT_GREEN; break;
                default: color = TFT_RED; break;
            }
        }

        tft.fillRect(DISPLAY_LED_SPC, DISPLAY_LED_SPC,
            DISPLAY_LED_SZ, DISPLAY_LED_SZ, color);
        display->state_led_touched = OS_FALSE;
    }

/*     if (display->state_led_on)
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
   */
}

#endif
