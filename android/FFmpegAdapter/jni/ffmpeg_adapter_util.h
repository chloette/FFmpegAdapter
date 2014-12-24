#ifndef __INCLUDE_FFMPEG_ADAPTER_UTIL_H__
#define __INCLUDE_FFMPEG_ADAPTER_UTIL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "ffmpeg_adapter_common.h"

#define ALIGN2_16(x) ((((x) + 15) >> 4) << 4)
#define ROUND(x) ((x) >= 0 ? (long)((x) + 0.5) : (long)((x) - 0.5))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) > (b) ? (a) : (b))
#define CHK_RES(x) \
  { \
    if (x) { \
      LOGE("%s %d Error: %s", __func__, __LINE__, get_error_text(x)); \
      goto EXIT; \
    } \
  } \

#define RELEASE_FRAME(x) \
  { \
    if (x) { \
      av_frame_free(&x); \
      x = NULL; \
    } \
  } \


#define RELEASE_SAMPLE_PP_BUFFER(x) \
  { \
  if (x) { \
    if (x[0]) { \
      av_freep(&(x[0])); \
    } \
    av_freep(&x); \
    } \
  } \

  /**
* Convert an error code into a text message.
* @param error Error code to be converted
* @return Corresponding error text (not thread-safe)
*/
char* const get_error_text(const int error);
/* check that a given sample format is supported by the encoder */
int check_sample_fmt(AVCodec *codec, enum AVSampleFormat sample_fmt);
/* just pick the highest supported samplerate */
int select_sample_rate(AVCodec *codec);
/* select layout with the highest channel count */
int select_channel_layout(AVCodec *codec);

int get_format_from_sample_fmt(const char **fmt, enum AVSampleFormat sample_fmt);

void fill_samples(double *dst, int nb_samples, int nb_channels, int sample_rate, double *t);

double sample_get(uint8_t** data, int channel, int index, int channel_count, enum AVSampleFormat f);

void  sample_set(uint8_t** data, int channel, int index, int channel_count, enum AVSampleFormat f, double value);

void sample_scale(uint8_t** data, int channel_count, int nb_samples, enum AVSampleFormat format, double ratio);

int scale_audio_frame_volume(AVFrame* audio_frame, double ratio);

void dump_to_file(const char* name, uint8_t* buffer, long size);

const char* generate_path_name(const char* root_path_name, const char* file_name);

int concat_files(const char** file_paths, const int paths_count, const char* output_path);

#ifdef __ANDROID__
#include <sys/time.h>
inline static uint64_t system_timestamp() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec*(uint64_t)1000000 + tv.tv_usec;
}
#else
#include <time.h>
inline static uint64_t system_timestamp() {
  return av_gettime();
}
#endif //__ANDROID__

#ifdef __cplusplus
}
#endif

#endif //__INCLUDE_FFMPEG_ADAPTER_UTIL_H__
