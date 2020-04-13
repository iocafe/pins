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


void pins_store_image_to_buffer(
    pinsPhoto *image,
    iocBrickBuffer *b,
    iocBrickCompression compression)
{
    os_memsz bytes;

   /* If we have not allocated brick buffer, do it
     */
    if (b->buf_sz == 0)
    {
        ioc_allocate_brick_buffer(b,
            image->iface->get_parameter(image->camera, PINS_CAM_MAX_IMAGE_SZ));
    }

    b->buf_n = 0;
    b->pos = 0;

    /* Compress brick data, set timestamp and calculate checksum Set current position to zero
     */
    bytes = ioc_compress_brick(b->buf, b->buf_sz,
        image->buf, image->format, image->w, image->h, compression);
    ioc_set_brick_timestamp(b->buf);
    ioc_set_brick_checksum(b->buf, b->buf_sz);

    b->buf_n = bytes;
}
