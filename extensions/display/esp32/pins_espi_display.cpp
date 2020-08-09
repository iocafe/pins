/**

  @file    extensions/display/esp32/pins_espi_display.c
  @brief   SPI TFT display wrapper for ESP/Arduino platform.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    21.4.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "pinsx.h"
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
    const pinsDisplayParams *prm)
{
    tft.init();

    tft.fillScreen(TFT_BLACK);
    display->title_touched = OS_TRUE;
    os_get_timer(&display->title_timer);
    display->displayed_code = MORSE_UNKNOWN;
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
    uint16_t background, color, bgr_color;
    osalMorseCodeEnum code;
    os_char buf[IOC_NETWORK_NAME_SZ], nbuf[OSAL_NBUF_SZ];
    const os_char *text;
    os_short x, y, r;
    os_boolean delay_it;

#define DISPLAY_W 240
#define DISPLAY_H 240
#define DISPLAY_TITLE_H 34
#define DISPLAY_SEPARATOR_H 3
#define DISPLAY_LED_SZ 16
#define DISPLAY_LED_SPC ((DISPLAY_TITLE_H - DISPLAY_LED_SZ) / 2)
#define DISPLAY_WARN_BOX_SZ 70

    if (os_has_elapsed_since(&display->title_timer, timer, 3000))
    {
        display->title_touched = OS_TRUE;
        display->show_network_name = !display->show_network_name;
        display->title_timer = *timer;
    }

    /* Some error conditions may blink off when retrying, delay when OK if signaled.
     */
    code = display->code;
    delay_it = OS_FALSE;
    if (display->displayed_code == MORSE_NO_CONNECTED_SOCKETS) {
         if (code == MORSE_RUNNING)
         {
            if (!os_has_elapsed_since(&display->code_change_timer, timer, 3000)) {
                delay_it = OS_TRUE;
            }
         }
         else
         {
             display->code_change_timer = *timer;
         }
    }

    /* Draw title background.
     */
    background = TFT_BLACK;
    if (display->title_touched)
    {
        tft.fillRect(0, 0, DISPLAY_W, DISPLAY_TITLE_H, background);
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
            tft.setTextColor(TFT_WHITE, background);
            tft.drawString(buf, DISPLAY_W / 2, DISPLAY_TITLE_H / 2, 4);
        }

        display->state_led_touched = OS_TRUE;
        display->title_touched = OS_FALSE;
    }

    if (display->state_led_touched)
    {
        color = background;
        if (display->state_led_on) {
            switch (code)
            {
                case MORSE_PROGRAMMING_DEVICE:
                case MORSE_CONFIGURING: color = TFT_YELLOW; break;
                case MORSE_CONFIGURATION_MATCH: color = TFT_GREEN; break;
                case MORSE_RUNNING: color = TFT_GREEN; break;
                default: color = TFT_RED; break;
            }
        }

        tft.fillRect(DISPLAY_LED_SPC, DISPLAY_LED_SPC,
            DISPLAY_LED_SZ, DISPLAY_LED_SZ, color);
        display->state_led_touched = OS_FALSE;
    }


    if (display->displayed_code != code && !delay_it)
    {
        display->app_rect_top = DISPLAY_TITLE_H + DISPLAY_SEPARATOR_H;
        if (code != MORSE_RUNNING)
        {
            if (code != MORSE_RUNNING)
            {
                tft.fillRect(0, display->app_rect_top, DISPLAY_W,
                    DISPLAY_WARN_BOX_SZ, background);
                tft.fillRect(0, display->app_rect_top + DISPLAY_WARN_BOX_SZ,
                    DISPLAY_W, DISPLAY_SEPARATOR_H, TFT_PURPLE);

                x = DISPLAY_W - DISPLAY_WARN_BOX_SZ/2;
                y = display->app_rect_top + DISPLAY_WARN_BOX_SZ/2;
                r = DISPLAY_WARN_BOX_SZ / 3;

                if (code == MORSE_CONFIGURATION_MATCH)
                {
                    bgr_color = TFT_GREEN;
                    color = TFT_BLUE;
                }
                else if (code > 0)
                {
                    bgr_color = TFT_YELLOW;
                    color = TFT_RED;
                }
                else
                {
                    bgr_color = TFT_SKYBLUE;
                    color = TFT_BLUE;
                }

                tft.fillCircle(x, y, r, bgr_color);
                tft.drawCircle(x, y, r, color);
                if (code > 0)
                {
                    osal_int_to_str(nbuf, sizeof(nbuf), code);
                    tft.setTextDatum(CC_DATUM);
                    tft.setTextColor(color, bgr_color);
                    tft.drawString(nbuf, x, y, 4);
                }

                text = morse_code_to_text(code);

                tft.setCursor(0, display->app_rect_top + 8, 2);
                tft.setTextColor(TFT_WHITE, TFT_BLACK);
                tft.println(text);

                switch (code)
                {
                    case MORSE_NETWORK_NOT_CONNECTED:
                        osal_get_network_state_str(OSAL_NS_WIFI_NETWORK_NAME, 0, buf, sizeof(buf));
                        if (buf[0] != '\0')
                        {
                            tft.print("wifi network: \"");
                            tft.print(buf);
                            tft.println("\"");
                        }
                        break;

                    case MORSE_NO_LIGHTHOUSE_FOR_THIS_IO_NETWORK:
                        if (display->root)
                        {
                            tft.print("IO network: \"");
                            tft.print(display->root->network_name);
                            tft.println("\"");
                        }
                        break;

                    default:
                        break;
                }

                display->app_rect_top += DISPLAY_WARN_BOX_SZ + DISPLAY_SEPARATOR_H;
            }
        }

        display->app_data_touched = OS_TRUE;
        display->displayed_code = code;
    }

    if (display->app_data_touched)
    {
        tft.fillRect(0, display->app_rect_top, DISPLAY_W, DISPLAY_H - display->app_rect_top, background);
        display->app_data_touched = OS_FALSE;
    }
}

#endif
