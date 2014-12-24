#ifndef __INCLUDE_FFMPEG_ADAPTER_TRANSFORM_H__
#define __INCLUDE_FFMPEG_ADAPTER_TRANSFORM_H__

#include "ffmpeg_adapter_common.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#define I_TABLE_SIZE (256)

  enum {
    YUV_FIX = 16,                // fixed-point precision
    YUV_RANGE_MIN = -227,        // min value of r/g/b output
    YUV_RANGE_MAX = 256 + 226    // max value of r/g/b output
  };

  extern int16_t ItableVToR[I_TABLE_SIZE], ItableUToB[I_TABLE_SIZE];
  extern int32_t ItableVToG[I_TABLE_SIZE], ItableUToG[I_TABLE_SIZE];
  extern uint8_t ItableClip[YUV_RANGE_MAX - YUV_RANGE_MIN];
  extern int16_t Itable257[I_TABLE_SIZE], Itable504[I_TABLE_SIZE], Itable098[I_TABLE_SIZE], Itable148[I_TABLE_SIZE], Itable291[I_TABLE_SIZE], Itable439[I_TABLE_SIZE], Itable368[I_TABLE_SIZE], Itable071[I_TABLE_SIZE];

  inline static void YuvToRgb(uint8_t y, uint8_t u, uint8_t v,
    uint8_t* const rgb) {
    const int r_off = ItableVToR[v];
    const int g_off = (ItableVToG[v] + ItableUToG[u]) >> YUV_FIX;
    const int b_off = ItableUToB[u];
    rgb[0] = ItableClip[y + r_off - YUV_RANGE_MIN];
    rgb[1] = ItableClip[y + g_off - YUV_RANGE_MIN];
    rgb[2] = ItableClip[y + b_off - YUV_RANGE_MIN];
  }

  inline static void YuvToRgba(int y, int u, int v, uint8_t* const rgba) {
    YuvToRgb(y, u, v, rgba);
    rgba[3] = 0xff;
  }

  inline static void YuvToBgr(uint8_t y, uint8_t u, uint8_t v,
    uint8_t* const bgr) {
    const int r_off = ItableVToR[v];
    const int g_off = (ItableVToG[v] + ItableUToG[u]) >> YUV_FIX;
    const int b_off = ItableUToB[u];
    bgr[0] = ItableClip[y + b_off - YUV_RANGE_MIN];
    bgr[1] = ItableClip[y + g_off - YUV_RANGE_MIN];
    bgr[2] = ItableClip[y + r_off - YUV_RANGE_MIN];
  }

  inline static void VP8YuvToBgra(int y, int u, int v, uint8_t* const bgra) {
    YuvToBgr(y, u, v, bgra);
    bgra[3] = 0xff;
  }

  inline static void RgbToY(uint8_t* rgb, uint8_t* const y) {
    *y = Itable257[rgb[0]] + Itable504[rgb[1]] + Itable098[rgb[2]] + 16;
  }

  inline static void RgbToU(uint8_t* rgb, uint8_t* const u) {
    *u = Itable148[rgb[0]] + Itable291[rgb[1]] + Itable439[rgb[2]] + 128;
  }

  inline static void RgbToV(uint8_t* rgb, uint8_t* const v) {
    *v = Itable439[rgb[0]] + Itable368[rgb[1]] + Itable071[rgb[2]] + 128;
  }

  inline static void RgbToYuv(uint8_t* rgb, uint8_t* const y, uint8_t* const u, uint8_t* const v) {
    RgbToY(rgb, y);
    RgbToU(rgb, u);
    RgbToV(rgb, v);
  }

  inline static void RgbaToYuv(uint8_t* rgba, uint8_t* const y, uint8_t* const u, uint8_t* const v) {
    RgbToYuv(rgba, y, u, v);
  }


  // Must be called before everything, to initialize the tables.
  void TransformTableInit();

#if defined(__cplusplus) || defined(c_plusplus)
}    // extern "C"
#endif

#endif //__INCLUDE_FFMPEG_ADAPTER_TRANSFORM_H__
