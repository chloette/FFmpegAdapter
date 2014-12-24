#include "ffmpeg_adapter_util.h"

#define TAG ("ffmpeg_adapter_util")

/**
* Convert an error code into a text message.
* @param error Error code to be converted
* @return Corresponding error text (not thread-safe)
*/
char *const get_error_text(const int error)
{
  static char error_buffer[255];
  av_strerror(error, error_buffer, sizeof(error_buffer));
  return error_buffer;
}

int check_sample_fmt(AVCodec *codec, enum AVSampleFormat sample_fmt)
{
  const enum AVSampleFormat *p = codec->sample_fmts;
  while (*p != AV_SAMPLE_FMT_NONE) {
    if (*p == sample_fmt)
      return 1;
    p++;
  }
  return 0;
}

/* just pick the highest supported samplerate */
int select_sample_rate(AVCodec *codec)
{
  const int *p;
  int best_samplerate = 0;
  if (!codec->supported_samplerates)
    return 44100; //TODO
  p = codec->supported_samplerates;
  while (*p) {
    best_samplerate = FFMAX(*p, best_samplerate);
    p++;
  }
  return best_samplerate;
}

/* select layout with the highest channel count */
int select_channel_layout(AVCodec *codec)
{
  const uint64_t *p;
  uint64_t best_ch_layout = 0;
  int best_nb_channels = 0;
  if (!codec->channel_layouts)
    return AV_CH_LAYOUT_STEREO;
  p = codec->channel_layouts;
  while (*p) {
    int nb_channels = av_get_channel_layout_nb_channels(*p);
    if (nb_channels > best_nb_channels) {
      best_ch_layout = *p;
      best_nb_channels = nb_channels;
    }
    p++;
  }
  return best_ch_layout;
}

int get_format_from_sample_fmt(const char **fmt,
enum AVSampleFormat sample_fmt)
{
  int i;
  struct sample_fmt_entry {
    enum AVSampleFormat sample_fmt; const char *fmt_be, *fmt_le;
  } sample_fmt_entries[] = {
    { AV_SAMPLE_FMT_U8, "u8", "u8" },
    { AV_SAMPLE_FMT_S16, "s16be", "s16le" },
    { AV_SAMPLE_FMT_S32, "s32be", "s32le" },
    { AV_SAMPLE_FMT_FLT, "f32be", "f32le" },
    { AV_SAMPLE_FMT_DBL, "f64be", "f64le" },
  };
  *fmt = NULL;

  for (i = 0; i < FF_ARRAY_ELEMS(sample_fmt_entries); i++) {
    struct sample_fmt_entry *entry = &sample_fmt_entries[i];
    if (sample_fmt == entry->sample_fmt) {
      *fmt = AV_NE(entry->fmt_be, entry->fmt_le);
      return 0;
    }
  }

  fprintf(stderr,
    "Sample format %s not supported as output format\n",
    av_get_sample_fmt_name(sample_fmt));
  return AVERROR(EINVAL);
}

/**
* Fill dst buffer with nb_samples, generated starting from t.
*/
void fill_samples(double *dst, int nb_samples, int nb_channels, int sample_rate, double *t)
{
  int i, j;
  double tincr = 1.0 / sample_rate, *dstp = dst;
  const double c = 2 * M_PI * 440.0;

  /* generate sin tone with 440Hz frequency and duplicated channels */
  for (i = 0; i < nb_samples; i++) {
    *dstp = sin(c * *t);
    for (j = 1; j < nb_channels; j++)
      dstp[j] = dstp[0];
    dstp += nb_channels;
    *t += tincr;
  }
}

double sample_get(uint8_t** data, int channel, int index, int channel_count, enum AVSampleFormat format) {
  const uint8_t *p;
  if (av_sample_fmt_is_planar(format)){
    format = av_get_alt_sample_fmt(format, 0);
    p = data[channel];
  } else{
    p = data[0];
    index = channel + index*channel_count;
  }

  switch (format){
  case AV_SAMPLE_FMT_U8: return ((const uint8_t*)p)[index] / 127.0 - 1.0;
  case AV_SAMPLE_FMT_S16: return ((const int16_t*)p)[index] / 32767.0;
  case AV_SAMPLE_FMT_S32: return ((const int32_t*)p)[index] / 2147483647.0;
  case AV_SAMPLE_FMT_FLT: return ((const float  *)p)[index];
  case AV_SAMPLE_FMT_DBL: return ((const double *)p)[index];
  default: return 0;
  }
}

void  sample_set(uint8_t** data, int channel, int index, int channel_count, enum AVSampleFormat format, double value){
  uint8_t *p;
  if (av_sample_fmt_is_planar(format)){
    format = av_get_alt_sample_fmt(format, 0);
    p = data[channel];
  } else{
    p = data[0];
    index = channel + index*channel_count;
  }
  switch (format){
  case AV_SAMPLE_FMT_U8: ((uint8_t*)p)[index] = av_clip_uint8(lrint((value + 1.0) * 127));     break;
  case AV_SAMPLE_FMT_S16: ((int16_t*)p)[index] = av_clip_int16(lrint(value * 32767));         break;
  case AV_SAMPLE_FMT_S32: ((int32_t*)p)[index] = av_clipl_int32(lrint(value * 2147483647));    break;
  case AV_SAMPLE_FMT_FLT: ((float  *)p)[index] = value;                                      break;
  case AV_SAMPLE_FMT_DBL: ((double *)p)[index] = value;                                      break;
  default: return;
  }
}

int scale_audio_frame_volume(AVFrame* audio_frame, double ratio) {
  if (NULL == audio_frame || !audio_frame->data || !audio_frame->data[0] || !audio_frame->nb_samples || !audio_frame->channels || ratio < 0)
    return -1;

  sample_scale(audio_frame->data, audio_frame->channels, audio_frame->nb_samples, (AVSampleFormat)audio_frame->format, ratio);

  return 0;
}

void sample_scale(uint8_t** data, int channel_count, int nb_samples, enum AVSampleFormat format, double ratio) {
  uint8_t *p = NULL;
  int is_planar = av_sample_fmt_is_planar(format);
  if (is_planar) {
    format = av_get_alt_sample_fmt(format, 0);
  }

  int index = 0;
  for (int channel = 0; channel < channel_count; channel++){
    for (int i = 0; i < nb_samples; i++) {
      if (is_planar) {
        p = data[channel];
        index = i;
      } else{
        p = data[0];
        index = channel + i * channel_count;
      }

      switch (format){
        //TODO: use << or >> to replace *
      case AV_SAMPLE_FMT_U8: ((uint8_t*)p)[index] *= ratio; break;
      case AV_SAMPLE_FMT_S16: ((int16_t*)p)[index] *= ratio; break;
      case AV_SAMPLE_FMT_S32: ((int32_t*)p)[index] *= ratio; break;
      case AV_SAMPLE_FMT_FLT: ((float  *)p)[index] *= ratio; break;
      case AV_SAMPLE_FMT_DBL: ((double *)p)[index] *= ratio; break;
      default: return;
      }
    }
  }
}

void dump_to_file(const char* name, uint8_t* buffer, long size) {
#ifndef __ANDROID__
  FILE * pTempFile = fopen(name, "wb");
  fwrite(buffer, sizeof(uint8_t), size / sizeof(uint8_t), pTempFile);
  fclose(pTempFile);
#else //__ANDROID__
  LOGE("---DUMP to--%s", name);
  int fd;
  fd = open(name, O_WRONLY | O_CREAT);
  write(fd, buffer, size);
  close(fd);
#endif//
}

const char* generate_path_name(const char* root_path_name, const char* file_name) {
  char buffer[256];
  sprintf(buffer, "%s%s", root_path_name, file_name);
  return buffer;
}

int concat_files(const char** input_file_paths, const int paths_count, const char* output_path) {
  int res = -1;
  AVOutputFormat *ofmt = NULL;
  AVFormatContext *ifmt_ctx = NULL, *ofmt_ctx = NULL;
  AVPacket pkt = { 0 };
  const char *current_in_filename = NULL;
  long addition_audio_pts = 0;
  long addition_audio_dts = 0;
  long addition_video_pts = 0;
  long addition_video_dts = 0;
  long last_audio_pts = 0;
  long last_audio_dts = 0;
  long last_audio_pkt_duration = 0;
  long last_video_pts = 0;
  long last_video_dts = 0;
  long last_video_pkt_duration = 0;
  AVStream *in_stream = NULL, *out_stream = NULL;

  if (!input_file_paths || paths_count <= 0 || !output_path) {
    res = ErrorParametersInvalid;
    goto EXIT;
  }

  av_register_all();

  res = avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, output_path);
  if (res < 0) {
    goto EXIT;
  }

  ofmt = ofmt_ctx->oformat;

  for (int i = 0; i < paths_count; i++) {
    res = avformat_open_input(&ifmt_ctx, input_file_paths[i], 0, 0);
    if (res < 0) {
      goto EXIT;
    }

    res = avformat_find_stream_info(ifmt_ctx, 0);
    if (res < 0) {
      goto EXIT;
    }

    while (1) {
      if (!ofmt_ctx->nb_streams) {
        for (int j = 0; j < ifmt_ctx->nb_streams; j++) {
          AVStream *in_stream = ifmt_ctx->streams[j];
          AVStream *out_stream = avformat_new_stream(ofmt_ctx, in_stream->codec->codec);
          if (!out_stream) {
            fprintf(stderr, "Failed allocating output stream\n");
            res = AVERROR_UNKNOWN;
            goto EXIT;
          }

          res = avcodec_copy_context(out_stream->codec, in_stream->codec);
          if (res < 0) {
            goto EXIT;
          }

          out_stream->codec->codec_tag = 0;
          if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
            out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
        }

        if (!(ofmt->flags & AVFMT_NOFILE)) {
          res = avio_open(&ofmt_ctx->pb, output_path, AVIO_FLAG_WRITE);
          if (res < 0) {
            goto EXIT;
          }
        }

        res = avformat_write_header(ofmt_ctx, NULL);
        if (res < 0) {
          goto EXIT;
        }
      }

      res = av_read_frame(ifmt_ctx, &pkt);
      if (res < 0)
        break;

      in_stream = ifmt_ctx->streams[pkt.stream_index];
      out_stream = ofmt_ctx->streams[pkt.stream_index];

      /* copy packet */

      pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
      pkt.pos = -1;

      if (in_stream->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
        pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, AVRounding(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX)) + addition_video_pts;
        pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, AVRounding(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX)) + addition_video_dts;

        last_video_pts = pkt.pts;
        last_video_dts = pkt.dts;
        last_video_pkt_duration = pkt.duration;
      } else {
        pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, AVRounding(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX)) + addition_audio_pts;
        pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, AVRounding(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX)) + addition_audio_dts;

        last_audio_pts = pkt.pts;
        last_audio_dts = pkt.dts;
        last_audio_pkt_duration = pkt.duration;
      }

      res = av_interleaved_write_frame(ofmt_ctx, &pkt);
      if (res < 0) {
        fprintf(stderr, "Error muxing packet\n");
        break;
      }

      av_free_packet(&pkt);
    }

    addition_audio_pts = last_audio_pkt_duration + last_audio_pts;
    addition_audio_dts = last_audio_pkt_duration + last_audio_dts;
    addition_video_pts = last_video_pkt_duration + last_video_pts;
    addition_video_dts = last_video_pkt_duration + last_video_dts;

    avformat_close_input(&ifmt_ctx);
  }

  res = av_write_trailer(ofmt_ctx);
  if (res) {
    goto EXIT;
  }

  res = 0;
EXIT:
  if (res) {

  }

  /* close output */
  if (ofmt_ctx) {
    if (!(ofmt->flags & AVFMT_NOFILE)) {
      avio_close(ofmt_ctx->pb);
    }

    avformat_free_context(ofmt_ctx);
  }

  if (ifmt_ctx) {
    avformat_close_input(&ifmt_ctx);
  }

  return res;
}
