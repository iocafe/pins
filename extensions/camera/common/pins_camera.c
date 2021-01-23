/**

  @file    extensions/camera/common/pins_camera.c
  @brief   Camera hardware API.
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
#if PINS_CAMERA

/**
****************************************************************************************************

   @brief Store a photo as a "brick" within brick buffer for communication.
   @anchor pins_store_photo_as_brick

   The pins_store_photo_as_brick() function...

   @param   photo Pointer to photo to store. The photo is not modified by this function.
   @param   b Pointer to brick buffer into which to store the photo as "brick".
   @param   compression Should photo be compressed by this function and is so how? 
   @return  OSAL_SUCCESS: photo was stored in buffer.
            OSAL_STATUS_OUT_OF_BUFFER: Photo didn't fit in buffer.
            Other return values: indicate an error.

****************************************************************************************************
*/
osalStatus pins_store_photo_as_brick(
    const pinsPhoto *photo,
    struct iocBrickBuffer *b,
    os_uchar compression)
{
    os_char quality;
    osalStatus s;

#if IOC_BRICK_RING_BUFFER_SUPPORT
    os_memsz bytes;

   /* If we have not allocated brick buffer, do it
     */
    bytes = photo->data_sz + sizeof(iocBrickHdr);
    bytes |= 0xFFF;
    if (bytes > b->buf_alloc_sz)
    {
        s = ioc_allocate_brick_buffer(b, bytes);
        if (s) return OSAL_STATUS_OUT_OF_BUFFER;
    }
#endif

    b->buf_n = 0;
    b->pos = 0;

    /* Compress brick data, set timestamp and calculate checksum Set current position to zero
     */
    s = ioc_compress_brick(b, photo->hdr, photo->data, photo->data_sz,
            photo->format, photo->w, photo->h, compression);

    /* If we are adjusting camera's JPEG compression quality accorting to compressed data size?
     */
    if (photo->iface != OS_NULL && photo->camera != OS_NULL) {
        quality = ioc_get_jpeg_compression_quality(b);
        if (quality > 0 && quality <= 100 && photo->iface->set_camera_jpeg_quality)
        {
            photo->iface->set_camera_jpeg_quality(photo->camera, quality);
        }
    }

    return s;
}


/**
****************************************************************************************************

   @brief Release camera information chain.
   @anchor pins_release_camera_info

   The pins_release_camera_info() function...

   @param camera_info Pointer to first item in camera info chain.
   @return None.

****************************************************************************************************
*/
void pins_release_camera_info(
    pinsCameraInfo *camera_info)
{
    pinsCameraInfo *next_camera_info;

    while (camera_info)
    {
        next_camera_info = camera_info->next;
        os_free(camera_info, sizeof(pinsCameraInfo));
        camera_info = next_camera_info;
    }
}

#endif
