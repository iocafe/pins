/**

  @file    extensions/detect_motion/common/pins_detect_motion.c
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

static void dm_show_debug_quarter_image(
    DetectMotion *dm,
    os_uchar *q_buf,
    os_uint movement,
    const struct pinsPhoto *photo);

/**
****************************************************************************************************

  @brief Initialize motion detection state structure.
  @anchor initialize_motion_detection

  The initialize_motion_detection() function initializes motion detection state structure.

  @param   detect_motion Motion detection state structure.
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

  @brief Release resources allocated for motion detection.
  @anchor release_motion_detection

  The release_motion_detection() function frees allocated memory.

  @param   detect_motion Motion detection state structure.
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


/* Return OSAL_NOTHING_TO_DO if frame can be skipped.
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
    if (!os_has_elapsed_since(&dm->image_set_ti, &ti, prm->min_interval_ms)) {
        return OSAL_NOTHING_TO_DO;
    }

    dm->q_w = photo->w/4;
    dm->q_h = photo->h/4;
    dm->h_w = 2 * dm->q_w;
    dm->h_h = 2 * dm->q_h;

    s = dm_allocate_all_buffers(dm);
    if (s) return s;

    s = dm_scale_original_image(dm, photo);
    if (s) return s;

    dm_blur(dm->h_buf1, dm->h_buf2, dm->h_w, dm->h_h);

    dm->q_new_sum = dm_scale_down(dm, dm->h_buf2, dm->q_new);

    movement = dm_calc_movement(dm);

    dm_show_debug_quarter_image(dm, dm->q_new, movement, photo);

    if (movement < prm->movement_limit &&
        !os_has_elapsed_since(&dm->image_set_ti, &ti, prm->max_interval_ms))
    {
        return OSAL_NOTHING_TO_DO;
    }

    os_memcpy(dm->q_prev, dm->q_new, dm->q_w * dm->q_h);
    dm->q_prev_sum = dm->q_new_sum;
    dm->image_set_ti = ti;

    return OSAL_SUCCESS;
}

void set_motion_detection_image(
    DetectMotion *dm,
    const struct pinsPhoto *photo)
{
    MotionDetectionParameters prm;
    MotionDetectionResults res;
    os_memclear(&prm, sizeof(prm));
    detect_motion(dm, photo, &prm, &res);
}


/* Make sure that all buffers are big enough to hold buf_sz bytes.
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

static osalStatus dm_scale_original_image(
    DetectMotion *dm,
    const struct pinsPhoto *photo)
{
    os_uchar *s, *s2, *d;
    os_int y, count, count2, h_w, h_h, byte_w;
    os_uint sum;

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
        s = photo->data + 2 * y * byte_w;
        d = dm->h_buf1 + y * h_w;
        count = h_w;
        while (count--) {
            sum = 0;
            s2 = s + byte_w;
            count2 = 6;
            while (count2--) {
                sum += *(s++);
            }
            count2 = 6;
            while (count2--) {
                sum += *(s2++);
            }

            *(d++) = (os_uchar)(sum / 12);
        }
    }

    return OSAL_SUCCESS;
}

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


static os_int dm_calc_movement(
    DetectMotion *dm)
{
    os_uchar *n, *p;
    os_uint new_coff, prev_coff, dn, dp;
    os_ulong total_sum;
    os_int count, delta;

    count = dm->q_w * dm->q_h;
    total_sum = dm->q_new_sum;
    if (total_sum <= 0) total_sum = 1;
    new_coff = 256 * count / total_sum;
    total_sum = dm->q_prev_sum;
    if (total_sum <= 0) total_sum = 1;
    prev_coff = 256 * count / total_sum;

    n = dm->q_new;
    p = dm->q_prev;
    total_sum = 0;

    while (count--)
    {
        dp = *(p++) * prev_coff;
        dn = *(n++) * new_coff;

        delta = (os_int)dp - (os_int)dn;
        delta *= delta;
        if (delta > 16) {
            total_sum += (os_uint)delta;
        }
    }

    total_sum /= dm->q_w * dm->q_h;
    return (os_int)total_sum;
}


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

static void dm_show_debug_quarter_image(
    DetectMotion *dm,
    os_uchar *q_buf,
    os_uint movement,
    const struct pinsPhoto *photo)
{
    os_uchar *s, *d, v, m;
    os_int y, count, q_w, q_h, byte_w;

    /* If source photo is something not supported, return error.
     */
    if (photo->format != OSAL_RGB24 ||
        photo->compression != IOC_UNCOMPRESSED ||
        photo->w < 16)
    {
        return;
    }

    q_w = dm->q_w;
    q_h = dm->q_h;
    byte_w = photo->byte_w;

    movement *= 5;

    for (y = 0; y < q_h; y++)
    {
        s = q_buf + y * q_w;
        d = photo->data + y * byte_w;
        count = q_w;
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
