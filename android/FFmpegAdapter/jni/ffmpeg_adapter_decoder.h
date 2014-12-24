#ifndef __INCLUDE_FFMPEG_ADAPTER_DECODER_H__
#define __INCLUDE_FFMPEG_ADAPTER_DECODER_H__

#include "ffmpeg_adapter_common.h"
#include "ffmpeg_adapter_converter.h"
/*
Support Audio ONLY.
*/

class ffmpeg_adapter_decoder {
public:
  ffmpeg_adapter_decoder();

  virtual ~ffmpeg_adapter_decoder();

  int is_started();

  int start(const char* filename);

  void end();

  bool is_eof();

  AVFrame* get_audio_frame();

  AVFrame* get_video_frame();

  long get_video_timestamp();

  long get_audio_timestamp();

  long get_video_duration();

  long get_audio_duration();

  int seek_to(long timestamp);

  int set_output_video_resolution(int width, int height);

  int set_output_video_format(int format);

private:

  int set_output_video_parameters(int width, int height, int format);

public:

  //get
  int get_audio_channels();

  int get_audio_channel_layout();

  int get_audio_format();

  int get_audio_samplerate();

  int get_audio_bitrate();

  int get_audio_framesize();

  int get_output_audio_buffer_size();

  //set

  int set_output_audio_channels(int channels);

  int set_output_audio_frame_size(int frame_size);

  int set_output_samplerate(int samplerate);

  int set_output_audio_format(int format);

private:

  int set_output_audio_parameters(int channels, int frame_size, int samplerate, int format);

  bool is_audio_convert_needed(AVFrame* p_new_frame);

private:
  // video stream context
  AVStream* m_p_video_stream;
  // audio streams context
  AVStream* m_p_audio_stream;

  AVFormatContext* m_p_fmt_ctx;

  AVCodecID m_video_codec_id;
  AVCodecID m_audio_codec_id;

  ffmpeg_adapter_converter* m_p_converter;

  int m_output_audio_samplerate;
  int m_output_audio_format;
  int m_output_audio_channels;
  int m_output_audio_frame_size;
  
  int m_output_video_width;
  int m_output_video_height;
  int m_output_video_format;

  long m_video_timestamp;
  long m_audio_timestamp;

  bool m_file_eof;

  int m_started;
};

#endif //__INCLUDE_FFMPEG_ADAPTER_DECODER_H__
