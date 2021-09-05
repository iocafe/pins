/**

  @file    detect_motion/common/pins_detect_motion.c
  @brief   Pins library extension for motion detection from camera image.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    3.9.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "pinsx.h"
#if PINS_CAMERA

/* Do we want to show motion detection debug image, define 0 or 1.
 */
#define DM_DEBUG_IMAGE 1


/* Forward referred static functions.
 */
static osalStatus dm_allocate_all_buffers(
    DetectMotion *dm);

static osalStatus dm_scale_original_image(
    DetectMotion *dm,
    const struct pinsPhoto *photo);

static os_ulong dm_scale_down(
    DetectMotion *dm,
    os_uchar *h_buf,
    os_uchar *q_buf);

static os_int dm_calc_movement(
    DetectMotion *dm);

static void dm_blur(
    os_uchar *src,
    os_uchar *dst,
    os_int w,
    os_int h);

static void dm_show_debug_image(
    os_uchar *src,
    os_int w,
    os_int h,
    os_uint movement,
    const struct pinsPhoto *photo);


/**
****************************************************************************************************

  @brief Initialize motion detection state structure.
  @anchor initialize_motion_detection

  The initialize_motion_detection() function initializes motion detection state structure.

  @param   dm Motion detection state structure.
  @return  None.

****************************************************************************************************
*/
void initialize_motion_detection(
    DetectMotion *dm)
{
    os_memclear(dm, sizeof(DetectMotion));
}


/**
****************************************************************************************************

  @brief Release memory allocated for motion detection.
  @anchor release_motion_detection

  The release_motion_detection() function frees allocated memory.

  @param   dm Motion detection state structure.
  @return  None.

****************************************************************************************************
*/
void release_motion_detection(
    DetectMotion *dm)
{
    os_free(dm->h_buf1, dm->h_buf1_alloc);
    os_free(dm->h_buf2, dm->h_buf2_alloc);
    os_free(dm->q_new, dm->q_new_alloc);
    os_free(dm->q_prev, dm->q_prev_alloc);
}


/**
****************************************************************************************************

  @brief Detect motion in camera image.
  @anchor detect_motion

  The detect_motion() function compares new photo to motion detection state and if motion
  is detected, the function returns OSAL_SUCCESS. The function updates motion detection
  state when motion is detected or maximum frame interval elapses.

  @param   dm Motion detection state structure.
  @param   photo New photo to check agains current movement detection state if there is movement.
           If movement is detected or max interval has been reached, the data of the new photo will
           be set as current motion detection state and the function returns OSAL_SUCCESS.
  @param   prm Parameters for motion detection.
  @param   res Motion detection results.
  @return  Return OSAL_NOTHING_TO_DO if frame can be skipped. OSAL_SUCCCESS indicates that
           there is movement, or the max frame interval limit have been reached. Other return
           values indicate error, etc, and should be treated as movement.

****************************************************************************************************
*/
osalStatus detect_motion(
    DetectMotion *dm,
    const struct pinsPhoto *photo,
    MotionDetectionParameters *prm,
    MotionDetectionResults *res)
{
    osalStatus s;
    os_timer ti;
    os_int movement;

    os_memclear(res, sizeof(MotionDetectionResults));

    /* Get timer reading for the new image. If not in parameters, use os_gettimer function.
       Check for minimum frame rate.
     */
    ti = prm->ti;
    if (ti == 0) {
        os_get_timer(&ti);
    }
    if (!os_has_elapsed_since(&dm->image_set_ti, &ti, prm->min_interval_ms) &&
        !dm->motion_trigger)
    {
        return OSAL_NOTHING_TO_DO;
    }

    /* We do not check motion on compressed images.
     */
    if (photo->compression & IOC_JPEG) {
        if (dm->motion_trigger) {
            dm->motion_trigger = OS_FALSE;
            return OSAL_SUCCESS;
        }
        goto getout;
    }

    dm->q_w = photo->w/8;
    dm->q_h = photo->h/8;
    dm->h_w = 2 * dm->q_w;
    dm->h_h = 2 * dm->q_h;

    s = dm_allocate_all_buffers(dm);
    if (s) return s;

    s = dm_scale_original_image(dm, photo);
    if (s) return s;

    dm_blur(dm->h_buf1, dm->h_buf2, dm->h_w, dm->h_h);

    dm->q_new_sum = dm_scale_down(dm, dm->h_buf2, dm->q_new);

    movement = dm_calc_movement(dm);
    res->movement = movement;

#if DM_DEBUG_IMAGE
    dm_show_debug_image(dm->q_new, dm->q_w, dm->q_h, movement, photo);
    // dm_show_debug_image(dm->h_buf1, dm->h_w, dm->h_h, movement, photo);
#endif

    if (movement < prm->movement_limit &&
        !os_has_elapsed_since(&dm->image_set_ti, &ti, prm->max_interval_ms) &&
        !dm->motion_trigger)
    {
        return OSAL_NOTHING_TO_DO;
    }

    /* Store new photo data as motion detection state.
     */
    os_memcpy(dm->q_prev, dm->q_new, dm->q_w * dm->q_h);
    dm->q_prev_sum = dm->q_new_sum;
    dm->motion_trigger = OS_FALSE;

getout:
    dm->image_set_ti = ti;
    return OSAL_SUCCESS;
}


/* Trigger motion: detect_motion returns motion immediately.
 */
void trigger_motion_detect(
    DetectMotion *dm)
{
    dm->motion_trigger = OS_TRUE;
}


/**
****************************************************************************************************

  @brief Allocate memory for motion detection image buffer.
  @anchor dm_allocate_buffer

  The dm_allocate_buffer function makes sure that all buffers are big enough to hold buf_sz bytes.
  Buffer is never shrunk.

  @param   bufptr Pointer to buffer pointer, can be modfied by this function.
  @param   byfsz Minimum number of bytes needed.
  @param   buf_alloc Pointer to allocated buffer size, can be modfied by this function.

  @return  If successfull, the function returns OSAL_SUCCESS. Return value
           OSAL_STATUS_MEMORY_ALLOCATION_FAILED indicates "out of memory".

****************************************************************************************************
*/
static osalStatus dm_allocate_buffer(
    os_uchar **bufptr,
    os_memsz buf_sz,
    os_memsz *buf_alloc)
{
    if (buf_sz > *buf_alloc) {
        if (*bufptr) {
            os_free(*bufptr, *buf_alloc);
        }
        *bufptr = (os_uchar*)os_malloc(buf_sz, buf_alloc);
        if (*bufptr == OS_NULL) {
            *buf_alloc = 0;
            return OSAL_STATUS_MEMORY_ALLOCATION_FAILED;
        }
    }

    return OSAL_SUCCESS;
}


/**
****************************************************************************************************

  @brief Allocate all motion detection buffers.
  @anchor dm_allocate_all_buffers

  The dm_allocate_all_buffers function makes sure that all enough memory has been allocated for
  low resolution motion detection buffers. Buffer is never shrunk.

  @param   dm Motion detection state structure.
  @return  If successfull, the function returns OSAL_SUCCESS. Return value
           OSAL_STATUS_MEMORY_ALLOCATION_FAILED indicates "out of memory".

****************************************************************************************************
*/
static osalStatus dm_allocate_all_buffers(
    DetectMotion *dm)
{
    os_memsz sz;

    sz = dm->h_w * dm->h_h;
    if (dm_allocate_buffer(&dm->h_buf1, sz, &dm->h_buf1_alloc)) {
        return OSAL_STATUS_MEMORY_ALLOCATION_FAILED;
    }
    if (dm_allocate_buffer(&dm->h_buf2, sz, &dm->h_buf2_alloc)) {
        return OSAL_STATUS_MEMORY_ALLOCATION_FAILED;
    }

    sz = dm->q_w * dm->q_h;
    if (dm_allocate_buffer(&dm->q_new, sz, &dm->q_new_alloc)) {
        return OSAL_STATUS_MEMORY_ALLOCATION_FAILED;
    }
    if (dm_allocate_buffer(&dm->q_prev, sz, &dm->q_prev_alloc)) {
        return OSAL_STATUS_MEMORY_ALLOCATION_FAILED;
    }

    return OSAL_SUCCESS;
}



/**
****************************************************************************************************

  @brief Convert photo down to low resolution grayscale image.
  @anchor dm_scale_original_image

  The dm_scale_original_image function makes scales original image down so that 4 x 4 pixel
  square is converted to 1 pixel in low resolution image h.

  For now, only uncompressed RGB is supported.

  @param   dm Motion detection state structure.
  @param   photo Photo to convert to low resolution.
  @return  If successfull, the function returns OSAL_SUCCESS. Return value
           OSAL_STATUS_NOT_SUPPORTED indicates that photo format or compression is not supported.

****************************************************************************************************
*/
static osalStatus dm_scale_original_image(
    DetectMotion *dm,
    const struct pinsPhoto *photo)
{
    os_uchar *s, *s2, *d;
    os_int y, count, count2, h_w, h_h, byte_w, i;
    os_uint sum;
    const os_int hor_bytes_for_pix = 12;
    const os_int ver_bytes_for_pix = 4;

    /* If source photo is something not supported, return error.
     */
    if (photo->format != OSAL_RGB24 ||
        photo->compression != IOC_UNCOMPRESSED ||
        photo->w < 16 ||
        photo->h < 16)
    {
        return OSAL_STATUS_NOT_SUPPORTED;
    }

    h_w = dm->h_w;
    h_h = dm->h_h;
    byte_w = photo->byte_w;

    for (y = 0; y < h_h; y++)
    {
        s2 = photo->data + 4 * y * byte_w;
        d = dm->h_buf1 + y * h_w;
        count = h_w;
        while (count--) {
            sum = 0;

            for (i = 0; i<ver_bytes_for_pix; i++)
            {
                s = s2 + i * byte_w;
                count2 = 12;
                while (count2--) {
                    sum += *(s++);
                }
            }

            *(d++) = (os_uchar)(sum / (hor_bytes_for_pix * ver_bytes_for_pix));
            s2 += hor_bytes_for_pix;
        }
    }

    return OSAL_SUCCESS;
}


/**
****************************************************************************************************

  @brief Scale H image down to Q image.
  @anchor dm_scale_down

  The dm_scale_down function scales H image to quarted resolution for Q-image.

  @param   dm Motion detection state structure.
  @param   h_buf Pointer to source image.
  @param   q_buf Target image buffer.
  @return  Sum of pixel values in Q-image.

****************************************************************************************************
*/
static os_ulong dm_scale_down(
    DetectMotion *dm,
    os_uchar *h_buf,
    os_uchar *q_buf)
{
    os_uchar *s, *d;
    os_int y, count, q_w, q_h, h_w;
    os_uint sum;
    os_ulong total_sum;

    q_w = dm->q_w;
    q_h = dm->q_h;
    h_w = dm->h_w;
    total_sum = 0;

    for (y = 0; y < q_h; y++)
    {
        s = h_buf + 2 * y * h_w;
        d = q_buf + y * q_w;
        count = q_w;
        while (count--) {
            sum = (os_uint)s[0] + (os_uint)s[1] + (os_uint)s[h_w] + (os_uint)s[h_w+1];
            s += 2;
            *(d++) = (os_uchar)(sum / 4);
            total_sum += sum;
        }
    }

    return total_sum / 4;
}


/**
****************************************************************************************************

  @brief Calculate how much movement there is in image.
  @anchor dm_calc_movement

  The dm_calc_movement function calculates for much movement there is in image. The Q-images
  average gray level is normalized. Difference between normalized pixel of Q-image in new image
  and saved Q-image is squared and summed.

  @param   dm Motion detection state structure.
  @return  Movement as number, 0 = no movement, >100 is much movement.

****************************************************************************************************
*/
static os_int dm_calc_movement(
    DetectMotion *dm)
{
    os_uchar *n, *p;
    os_uint new_coeff, prev_coeff, dn, dp;
    os_ulong total_sum;
    os_int count, delta;

    count = dm->q_w * dm->q_h;
    total_sum = dm->q_new_sum;
    if (total_sum <= 0) total_sum = 1;
    new_coeff = (os_uint)(16535 * count / total_sum);
    total_sum = dm->q_prev_sum;
    if (total_sum <= 0) total_sum = 1;
    prev_coeff = (os_uint)(16535 * count / total_sum);

    /* Do not alert in darkness all the time.
     */
    if (new_coeff > 256) new_coeff = 256;
    if (prev_coeff > 256) prev_coeff = 256;

    n = dm->q_new;
    p = dm->q_prev;
    total_sum = 0;

    while (count--)
    {
        dp = *(p++) * prev_coeff;
        dn = *(n++) * new_coeff;

        delta = (os_int)dp - (os_int)dn;
        delta *= delta;
        if (delta > 16) {
            total_sum += (os_uint)delta;
        }
    }

    total_sum /= dm->q_w * dm->q_h * 64 * 64;
    return (os_int)total_sum;
}


/**
****************************************************************************************************

  @brief Blur image.
  @anchor dm_blur

  The dm_blur function creates blurred copy of original image in another buffer. Both source and
  result images are grayscale images and do have the same size.

  @param   src Pointer to source image data.
  @param   dst Pointer to destination image buffer.
  @param   w Image width in pixels.
  @param   h Image height in pixels.
  @return  None.

****************************************************************************************************
*/
static void dm_blur(
    os_uchar *src,
    os_uchar *dst,
    os_int w,
    os_int h)
{
    os_uchar *s, *d;
    os_int x, y;

    dst[0] = ((os_uint)src[0] + (os_uint)src[1] + (os_uint)src[w]) / 3;
    dst[w-1] = ((os_uint)src[w-2] + (os_uint)src[w-1] + (os_uint)src[2*w-1]) / 3;
    for (x = 1; x < w-1; x++) {
        dst[x] = ((os_uint)src[x-1] + (os_uint)src[x] + (os_uint)src[x+1] + (os_uint)src[x+w]) / 4;
    }

    for (y = 1; y < h-1; y++) {
        s = src + y * w;
        d = dst + y * w;

        d[0] = ((os_uint)s[0] + (os_uint)s[1] + (os_uint)s[-w] + (os_uint)s[w]) / 4;
        d[w-1] = ((os_uint)s[w-2] + (os_uint)s[w-1] + (os_uint)s[-1] + (os_uint)s[2*w-1]) / 4;

        for (x = 1; x < w-1; x++) {
            d[x] = ((os_uint)s[x-1] + (os_uint)s[x] + (os_uint)s[x+1] + (os_uint)s[x+w] + (os_uint)s[x-w]) / 5;
        }
    }

    s = src + (h-1) * w;
    d = dst + (h-1) * w;

    d[0] = ((os_uint)s[0] + (os_uint)s[1] + (os_uint)s[-w]) / 3;
    d[w-1] = ((os_uint)s[w-2] + (os_uint)s[w-1] + (os_uint)s[-1]) / 3;
    for (x = 1; x < w-1; x++) {
        d[x] = ((os_uint)s[x-1] + (os_uint)s[x] + (os_uint)s[x+1] + (os_uint)s[x-w]) / 4;
    }
}

#if DM_DEBUG_IMAGE
/**
****************************************************************************************************

  @brief Blur image.
  @anchor dm_show_debug_image

  The dm_show_debug_image function draws a low resolution image on top of original photo.
  This is used to visualize motion detection images while debugging.

  @param   src Pointer to source image data.
  @param   w Image width in pixels.
  @param   h Image height in pixels.
  @param   movement Movement value to draw as solid color component.
  @param   photo Photo on top of which small low resulution debug image is drawn.
  @return  None.

****************************************************************************************************
*/
static void dm_show_debug_image(
    os_uchar *src,
    os_int w,
    os_int h,
    os_uint movement,
    const struct pinsPhoto *photo)
{
    os_uchar *s, *d, v, m;
    os_int y, count, byte_w;

    /* If source photo is something not supported, return error.
     */
    if (photo->format != OSAL_RGB24 ||
        photo->compression != IOC_UNCOMPRESSED ||
        photo->w < 16)
    {
        return;
    }

    byte_w = photo->byte_w;

    for (y = 0; y < h; y++)
    {
        s = src + y * w;
        d = photo->data + y * byte_w;
        count = w;
        while (count--) {
            v = *(s++);
            m = (os_uchar)(movement < 255 ? movement : 255);
            *(d++) = m;
            *(d++) = m;
            *(d++) = v;
        }
    }
}
#endif

#endif
