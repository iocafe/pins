/**

  @file    extensions/morse/common/pins_morse_text.c
  @brief   Convert morse code enum value to string.
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


/**
****************************************************************************************************

  @brief Get text describing a morse code.
  @anchor morse_code_to_text

  The morse_code_to_text() function...

  @param   code Morse code enumeration value.
  @return  Pointer to string describing the code.

****************************************************************************************************
*/
const os_char *morse_code_to_text(
    MorseCodeEnum code)
{
    const os_char *p;

    switch (code)
    {
        case MORSE_CONFIGURING: p = "configuring"; break;
        case MORSE_CONFIGURATION_MATCH: p = "configuration ready"; break;
        case MORSE_RUNNING: p = "running"; break;
        case MORSE_NETWORK_NOT_CONNECTED: p = "network not\nconnected"; break;
        case MORSE_LIGHTHOUSE_NOT_VISIBLE: p = "server multicast\nnot received"; break;
        case MORSE_NO_LIGHTHOUSE_FOR_THIS_IO_NETWORK: p = "no server multicast\nfor requested network"; break;
        case MORSE_SECURITY_CONF_ERROR: p = "security\nconfiguration error"; break;
        case MORSE_NO_CONNECTED_SOCKETS: p = "no connection\nto server"; break;
        case MORSE_DEVICE_INIT_INCOMPLETE: p = "device not\ninitialized"; break;
        default: p = "UNKNOWN"; break;
    }

    return p;
}
