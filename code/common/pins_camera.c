/**

  @file    common/pins_camera.c
  @brief   Camera hardware API.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    10.4.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "pins.h"

/* Store check sum within image
 */
void pins_set_camera_image_checksum(
    pinsCameraImage *image)
{
    pinsCameraImageBufHdr *hdr;
    os_ushort checksum;

    hdr = (pinsCameraImageBufHdr*)image->buf;
    hdr->checksum_low = 0;
    hdr->checksum_high = 0;
    checksum = os_checksum((const os_char*)image->buf, image->buf_sz, OS_NULL);
    hdr->checksum_low = (os_uchar)checksum;
    hdr->checksum_high = (os_uchar)(checksum >> 8);
}


/* Store time stamp within image (must be called before pins_set_camera_image_checksum)
 */
void pins_set_camera_image_timestamp(
    pinsCameraImage *image)
{
    pinsCameraImageBufHdr *hdr;
    os_timer ti;

    hdr = (pinsCameraImageBufHdr*)image->buf;
    os_get_timer(&ti);

#ifdef OSAL_SMALL_ENDIAN
    os_memcpy(hdr->tstamp, &ti, PINS_CAMERA_IMG_TSTAMP_SZ);
#else
    os_uchar *ss, *dd;
    int count;

    count = PINS_CAMERA_IMG_TSTAMP_SZ;
    ss = (os_uchar*)&ti;
    dd = (os_uchar*)hdr->tstamp;
    while (count--)
    {
        *(dd++) = ss[count];
    }
#endif
}
