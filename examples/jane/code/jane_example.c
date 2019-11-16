/**

  @file    pins/examples/jane/code/jane_example.c
  @brief   Very basic pins library example.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    17.9.2019

  Copyright 2012 - 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/

/* Force tracing on for this source file.
 */
#undef OSAL_TRACE
#define OSAL_TRACE 3

#include "jane.h"

/* Here we include hardware specific IO code. The file name is always same for jane, but
   pins/<hardware> is added compiler's include paths
 */
#include "jane-io.c"

os_timer t;
os_boolean state;
os_int dip3, dip4, touch, dimmer, dimmer_dir, potentiometer;

/**
****************************************************************************************************

  @brief Process entry point.

  The osal_main() function is OS independent entry point.

  @param   argc Number of command line arguments.
  @param   argv Array of string pointers, one for each command line argument. UTF8 encoded.

  @return  None.

****************************************************************************************************
*/
osalStatus osal_main(
    os_int argc,
    os_char *argv[])
{
    pins_setup(&pins_hdr, 0);

    os_get_timer(&t);
    state = OS_FALSE;
    dip3 = dip4 = touch = -1;
    dimmer = 0;
    dimmer_dir = 1;
    potentiometer = -4095;

    /* When emulating micro-controller on PC, run loop. Just save context pointer on
       real micro-controller.
     */
    osal_simulated_loop(OS_NULL);
    return 0;
}


/**
****************************************************************************************************

  @brief Loop function to be called repeatedly.

  The osal_loop() function...

  @param   app_context Void pointer, reserved to pass context structure, etc.
  @return  The function returns OSAL_SUCCESS to continue running. Other return values are
           to be interprened as reboot on micro-controller or quit the program on PC computer.

****************************************************************************************************
*/
osalStatus osal_loop(
    void *app_context)
{
    os_int x, delta;
    os_char buf[32];

    /* Digital output */
    if (os_elapsed(&t, 50))
    {
        os_get_timer(&t);
        state = !state;
        pin_set(&pins.outputs.led_builtin, state);
    }

    /* Digital input */
    x = pin_get(&pins.inputs.dip_switch_3);
    if (x != dip3)
    {
        dip3 = x;
        osal_console_write(dip3 ? "DIP switch 3 turned ON\n" : "DIP switch 3 turned OFF\n");
    }
    x = pin_get(&pins.inputs.dip_switch_4);
    if (x != dip4)
    {
        dip4 = x;
        osal_console_write(dip4 ? "DIP switch 4 turned ON\n" : "DIP switch 4 turned OFF\n");
    }

    /* Touch sensor */
    x = pin_get(&pins.inputs.touch_sensor);
    delta = touch - x;
    if (delta < 0) delta = -delta;
    if (delta > 20)
    {
        touch = x;
        if (touch)
        {
          osal_console_write("TOUCH_SENSOR: ");
          osal_int_to_string(buf, sizeof(buf), touch);
          osal_console_write(buf);
          osal_console_write("\n");
        }
    }

    /* Analog input */
    x = pin_get(&pins.analog_inputs.potentiometer);
    delta = potentiometer - x;
    if (delta < 0) delta = -delta;
    if (delta > 100)
    {
        potentiometer = x;
        osal_console_write("POTENTIOMETER: ");
        osal_int_to_string(buf, sizeof(buf), potentiometer);
        osal_console_write(buf);
        osal_console_write("\n");
    }

    /* PWM */
    dimmer += dimmer_dir;
    if (dimmer > 4095 || dimmer < 0) dimmer_dir = -dimmer_dir;
    pin_set(&pins.pwm.dimmer_led, dimmer);

    return OSAL_SUCCESS;
}


/**
****************************************************************************************************

  @brief Finished with the application, clean up.

  The osal_main_cleanup() function closes the stream, then closes underlying stream library.
  Notice that the osal_stream_close() function does close does nothing if it is called with NULL
  argument.

  @param   app_context Void pointer, reserved to pass context structure, etc.
  @return  None.

****************************************************************************************************
*/
void osal_main_cleanup(
    void *app_context)
{
}
