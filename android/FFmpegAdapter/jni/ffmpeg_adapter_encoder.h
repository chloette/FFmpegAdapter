#ifndef __INCLUDE_FFMPEG_ADAPTER_ENCODER_H__
#define __INCLUDE_FFMPEG_ADAPTER_ENCODER_H__

#include "ffmpeg_adapter_common.h"
#include "ffmpeg_adapter_converter.h"

class ffmpeg_adapter_encoder {
public:
  ffmpeg_adapter_encoder();

  virtual ~ffmpeg_adapter_encoder();

  //these parameters should be settled before start
  int set_output_path(const char* path);
  int set_video_extra_data(uint8_t* extradata, long length);
  int set_audio_extra_data(uint8_t* extradata, long length);
  int set_resolution(int width, int height);
  int set_video_quality(int quality);
  int set_fps(double fps);
  int set_audio_channels(int channels);
  int set_sample_format(int sample_fmt);
  int set_samplerate(int samplerate);
  int set_audio_bitrate(int bitrate);
  int enable_fake_audio(bool enable);
  int use_fake_audio();

  int is_started();
private:
  //no one needs this anymore (auto-start when encoding)
  int start();

  int check_start();

  int use_3rd_video_encoder();
  
  int use_3rd_audio_encoder();
public:
  void end();

  // Add video frame
  int add_video_frame(AVFrame* p_new_frame);
  int add_video_frame(AVFrame* p_new_frame, long timestamp);
  int add_compressed_video_frame(uint8_t* buffer, long length, long timestamp, int flag);

  //add audio frame
  int add_audio_frame(AVFrame* p_new_frame);
  int add_audio_frame(AVFrame* p_new_frame, long timestamp);
  int add_compressed_audio_frame(uint8_t* buffer, long length, long timestamp);

  int get_fps();

  long get_current_video_duration();

  long get_current_audio_duration();

public:
  //for inner test only
  void add_image_file(const char* file_name, int width, int height, int timestamp);

  // Allocate memory
  static AVFrame* create_ffmpeg_audio_frame(int nb_samples, int format, int channels, int channel_layout, int sample_rate);

  static AVFrame* create_ffmpeg_audio_frame(int nb_samples, int format, int channels, int channel_layout, int sample_rate, uint8_t* buffer, int buffer_length);

  static int create_ffmpeg_audio_frame(AVFrame **frame,
    AVCodecContext *output_codec_context,
    int frame_size);

  static int encode_audio_frame(AVFrame *frame,
    AVFormatContext *output_format_context,
    AVStream* output_audio_stream,
    bool interval,
    int *data_present);

  // Allocate memory
  static AVFrame* create_ffmpeg_video_frame(int pix_fmt, int width, int height);

  //please make sure all parameters match.
  static AVFrame* create_ffmpeg_video_frame(int pix_fmt, uint8_t* buffer, int width, int height);

private:
  // Open Video Stream
  int open_video_stream();

  int open_video_stream(uint8_t* extradata, long length);

  // Open audio stream
  int open_audio_stream();

  int open_audio_stream(uint8_t* extradata, long length);

  // Free resourses.
  void release();

private:
  char* m_p_file_path;

  uint8_t* m_p_video_extradata;
  int m_video_extradata_length;

  uint8_t* m_p_audio_extradata;
  int m_audio_extradata_length;

  // video stream context
  AVStream* m_p_video_stream;
  // audio streams context
  AVStream* m_p_audio_stream;

  AVFormatContext* m_p_fmt_ctx; //key context, used to judge init succeed or not

  ffmpeg_adapter_converter* m_p_converter;

  AVCodecID m_video_codec_id;
  AVCodecID m_audio_codec_id;

  AVPixelFormat m_pix_fmt;
  AVSampleFormat m_sample_fmt;
  int m_video_bitrate;
  int m_audio_bitrate;
  int m_samplerate;
  int m_audio_channels;

  double m_video_quality;
  double m_audio_quality;
  double m_frame_rate;

  int m_width;
  int m_height;

  int m_video_frame_count;
  int m_audio_frame_count;

  bool m_enable_fake_audio;
  AVFrame* m_fake_audio_frame;

  bool m_started;
};

#endif // __INCLUDE_FFMPEG_ADAPTER_ENCODER_H__
