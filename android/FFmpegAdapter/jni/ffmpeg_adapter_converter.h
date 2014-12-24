#ifndef __INCLUDE_FFMPEG_ADAPTER_CONVERTER_H__
#define __INCLUDE_FFMPEG_ADAPTER_CONVERTER_H__

#include "ffmpeg_adapter_common.h"

#define DEFAULT_AUDIO_SAMPLE_RATE (48000)

class ffmpeg_adapter_converter {
public:
  ffmpeg_adapter_converter();

  virtual ~ffmpeg_adapter_converter();
  
  int init_audio(
    int input_audio_samplerate,
    int input_audio_channels,
    int input_audio_format,
    int output_audio_samplerate,
    int output_audio_channels,
    int output_audio_format,
    int output_audio_frame_size);

  int init_video(
    int input_video_width,
    int input_video_height,
    int input_video_format,
    int output_video_width,
    int output_video_height,
    int output_video_format);

  int get_output_audio_buffer_size();

  int get_audio_fifo_size();

  int input_audio(AVFrame* p_new_frame);

  int output_audio(AVFrame** pp_new_frame, bool force);

  int convert_video(AVFrame** pp_new_frame, AVFrame* p_origin_frame);

  int is_converted_video_frame_owned();

  void enable_own_converted_video_frame(int own);

  static int convert_rgba_to_yv12(uint8_t* rgba_buffer, uint8_t* yuv_buffer, int width, int height);

  static int convert_rgba_to_nv12(uint8_t* rgba_buffer, uint8_t* yuv_buffer, int width, int height);

  static int convert_rgba_to_nv21(uint8_t* rgba_buffer, uint8_t* yuv_buffer, int width, int height);

  static int convert_yv21_to_rgba(uint8_t* yuv_buffer, uint8_t* rgba_buffer, int width, int height);

private:
  int init_audio_convert_ctx();

  void destroy_audio_convert_ctx();

  int init_video_convert_ctx();

  void destroy_video_convert_ctx();

private:
  SwrContext* m_p_sample_convert_ctx;
  AVAudioFifo* m_p_audio_fifo;
  uint8_t** m_p_converted_input_samples;
  int m_converted_input_samples_length;

  SwsContext* m_p_img_convert_ctx;
  AVFrame* m_p_temp_frame;
  uint8_t* m_dst_data[4];
  

  int m_own_converted_video_frame;

  int m_input_audio_samplerate;
  int m_input_audio_format;
  int m_input_audio_channels;

  int m_output_audio_samplerate;
  int m_output_audio_format;
  int m_output_audio_channels;
  int m_output_audio_frame_size;

  int m_input_video_width;
  int m_input_video_height;
  int m_input_video_format;

  int m_output_video_width;
  int m_output_video_height;
  int m_output_video_format;
};

#endif //__INCLUDE_FFMPEG_ADAPTER_CONVERTER_H__
