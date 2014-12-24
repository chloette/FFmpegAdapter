#include "ffmpeg_adapter_transform.h"

#define TAG ("ffmpeg_adapter_transform")

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

  enum { YUV_HALF = 1 << (YUV_FIX - 1) };

  int16_t ItableVToR[I_TABLE_SIZE], ItableUToB[I_TABLE_SIZE];
  int32_t ItableVToG[I_TABLE_SIZE], ItableUToG[I_TABLE_SIZE];
  uint8_t ItableClip[YUV_RANGE_MAX - YUV_RANGE_MIN];

  int16_t Itable257[I_TABLE_SIZE], Itable504[I_TABLE_SIZE], Itable098[I_TABLE_SIZE], Itable148[I_TABLE_SIZE], Itable291[I_TABLE_SIZE], Itable439[I_TABLE_SIZE], Itable368[I_TABLE_SIZE], Itable071[I_TABLE_SIZE];

  static int done = 0;

  /*
  Y' = 0.257*R' + 0.504*G' + 0.098*B' + 16
  Cb' = -0.148*R' - 0.291*G' + 0.439*B' + 128
  Cr' = 0.439*R' - 0.368*G' - 0.071*B' + 128
  R' = 1.164*(Y'-16) + 1.596*(Cr' - 128) = 1.164*(Y'-16 + 1.371*(Cr'-128) //vise versa
  G' = 1.164*(Y'-16) - 0.813*(Cr' - 128) - 0.392*(Cb'-128)
  B' = 1.164*(Y'-16) + 2.017*(Cb' - 128)
  */

  void TransformTableInit() {
    int i;
    if (done) {
      return;
    }
    for (i = 0; i < I_TABLE_SIZE; ++i) {
      ItableVToR[i] = (89858 * (i - 128) + YUV_HALF) >> YUV_FIX;
      ItableUToG[i] = -22014 * (i - 128) + YUV_HALF;
      ItableVToG[i] = -45773 * (i - 128) + YUV_HALF;
      ItableUToB[i] = (113618 * (i - 128) + YUV_HALF) >> YUV_FIX;

      Itable257[i] = (16842 * i + YUV_HALF) >> YUV_FIX;
      Itable504[i] = (33030 * i + YUV_HALF) >> YUV_FIX;
      Itable098[i] = (6422 * i + YUV_HALF) >> YUV_FIX;
      Itable148[i] = (-9699 * i + YUV_HALF) >> YUV_FIX;
      Itable291[i] = (-19070 * i + YUV_HALF) >> YUV_FIX;
      Itable439[i] = (28770 * i + YUV_HALF) >> YUV_FIX;
      Itable368[i] = (-24117 * i + YUV_HALF) >> YUV_FIX;
      Itable071[i] = (-4653 * i + YUV_HALF) >> YUV_FIX;
    }
    for (i = YUV_RANGE_MIN; i < YUV_RANGE_MAX; ++i) {
      const int k = ((i - 16) * 76283 + YUV_HALF) >> YUV_FIX;
      ItableClip[i - YUV_RANGE_MIN] = (k < 0) ? 0 : (k > I_TABLE_SIZE - 1) ? I_TABLE_SIZE - 1 : k;
    }
    done = 1;
  }

#if defined(__cplusplus) || defined(c_plusplus)
}    // extern "C"
#endif
