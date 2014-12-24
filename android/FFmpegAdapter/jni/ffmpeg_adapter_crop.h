/*
 *  It seems that you are not a coder if DO NOT start with Copyrights.
 *  What's wrong with you guys ? 
 *
 *  -- E
 */

#ifndef __INCLUDE_FFMPEG_ADAPTER_CROP_H__
#define __INCLUDE_FFMPEG_ADAPTER_CROP_H__

#include "ffmpeg_adapter_common.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

  //TODO to support Height crop

  //Y means Y of YUV
  //y means y of x & y
  inline static int YUV420P_crop(uint8_t* src, uint8_t* dst, int src_width, int src_height, int dst_x, int dst_y, int dst_width, int dst_height) {
    int src_Y_size = src_width * src_height;
    int dst_Y_size = dst_width * dst_height;
    int offset_Y_x = dst_x;
    int offset_Y_y = dst_y;
    int offset_uv_x = (dst_x >> 1);
    int offset_uv_y = (dst_y >> 1);
    int src_uv_width = (src_width >> 1);
    int src_uv_height = (src_height >> 1);
    int dst_uv_width = (dst_width >> 1);
    int dst_uv_height = (dst_height >> 1);
    int index = 0;

    uint8_t* src_cb = src + src_Y_size;
    uint8_t* src_cr = src + src_Y_size + (src_Y_size >> 2); // (Y * 5 / 4)
    uint8_t* dst_cb = dst + dst_Y_size;
    uint8_t* dst_cr = dst + dst_Y_size + (dst_Y_size >> 2); // (Y * 5 / 4)

    for (index = 0; index < src_height; index++) {
      //Y
      memcpy(dst + dst_width * index,
        src + src_width * index + offset_Y_x,
        dst_width);
    }

    for (index = 0; index < src_uv_height; index++) {
      //Cb
      memcpy(dst_cb + dst_uv_width * index,
        src_cb + src_uv_width * index + offset_uv_x,
        dst_uv_width);

      //Cr
      memcpy(dst_cr + dst_uv_width * index,
        src_cr + src_uv_width * index + offset_uv_x,
        dst_uv_width);
    }

    return 0;
  }

  inline static int YUV420SP_crop(uint8_t* src, uint8_t* dst, int src_width, int src_height, int dst_x, int dst_y, int dst_width, int dst_height) {
    int src_Y_size = src_width * src_height;
    int dst_Y_size = dst_width * dst_height;
    int offset_Y_x = dst_x;
    int offset_Y_y = dst_y;
    int offset_uv_x = (dst_x);
    int offset_uv_y = (dst_y >> 1);
    int src_uv_width = (src_width);
    int src_uv_height = (src_height >> 1);
    int dst_uv_width = (dst_width);
    int dst_uv_height = (dst_height >> 1);
    int index = 0;

    uint8_t* src_cbcr = src + src_Y_size;
    uint8_t* dst_cbcr = dst + dst_Y_size;

    for (index = 0; index < src_height; index++) {
      //Y
      memcpy(dst + dst_width * index,
        src + src_width * index + offset_Y_x,
        dst_width);
    }

    for (index = 0; index < src_uv_height; index++) {
      //CbCr
      memcpy(dst_cbcr + dst_uv_width * index,
        src_cbcr + src_uv_width * index + offset_uv_x,
        dst_uv_width);
    }

    return 0;
  }

#if defined(__cplusplus) || defined(c_plusplus)
}    // extern "C"
#endif

#endif  // __INCLUDE_FFMPEG_ADAPTER_CROP_H__
