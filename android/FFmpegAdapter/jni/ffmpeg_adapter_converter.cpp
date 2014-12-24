#include "ffmpeg_adapter_converter.h"
#include "ffmpeg_adapter_transform.h"
#include "ffmpeg_adapter_encoder.h"

#define TAG ("ffmpeg_adapter_converter")

#define MIN_CONVERT_BUFFER_SIZE (16384)

#define USE_PERFORMANCE_OPTIMIZATION (true)

ffmpeg_adapter_converter::ffmpeg_adapter_converter() {
  m_p_sample_convert_ctx = NULL;
  m_p_audio_fifo = NULL;
  m_p_converted_input_samples = NULL;
  m_converted_input_samples_length = 0;

  m_p_img_convert_ctx = NULL;
  m_p_temp_frame = NULL;
  m_own_converted_video_frame = 1;

  m_input_audio_samplerate = -1;
  m_input_audio_format = -1;
  m_input_audio_channels = -1;

  m_output_audio_samplerate = -1;
  m_output_audio_format = -1;
  m_output_audio_channels = -1;
  m_output_audio_frame_size = 0;

  m_input_video_width = 0;
  m_input_video_height = 0;
  m_input_video_format = -1;

  m_output_video_width = 0;
  m_output_video_height = 0;
  m_output_video_format = -1;
  memset(m_dst_data, 0, sizeof(m_dst_data[0]) * 4);
}

ffmpeg_adapter_converter::~ffmpeg_adapter_converter() {
  destroy_audio_convert_ctx();
  destroy_video_convert_ctx();
}

int ffmpeg_adapter_converter::init_audio(
  int input_audio_samplerate,
  int input_audio_channels,
  int input_audio_format,
  int output_audio_samplerate,
  int output_audio_channels,
  int output_audio_format,
  int output_audio_frame_size) {

  int res = -1;

  if (input_audio_samplerate <= -1 ||
    input_audio_format <= -1 ||
    input_audio_channels <= -1 ||
    output_audio_samplerate <= -1 ||
    output_audio_format <= -1 ||
    output_audio_channels <= -1 ||
    output_audio_frame_size <= 0) {
    res = ErrorParametersInvalid;
    goto EXIT;
  }

  if (m_input_audio_samplerate == input_audio_samplerate &&
    m_input_audio_format == input_audio_format &&
    m_input_audio_channels == input_audio_channels &&
    m_output_audio_samplerate == output_audio_samplerate &&
    m_output_audio_format == output_audio_format &&
    m_output_audio_channels == output_audio_channels &&
    m_output_audio_frame_size == output_audio_frame_size) {
    res = 0;
    goto EXIT;
  }

  destroy_audio_convert_ctx();

  m_input_audio_samplerate = input_audio_samplerate;
  m_input_audio_format = input_audio_format;
  m_input_audio_channels = input_audio_channels;

  m_output_audio_samplerate = output_audio_samplerate;
  m_output_audio_format = output_audio_format;
  m_output_audio_channels = output_audio_channels;

  m_output_audio_frame_size = output_audio_frame_size;

  res = init_audio_convert_ctx();
  if (res)
    goto EXIT;

  res = 0;
EXIT:
  if (res) {
    destroy_audio_convert_ctx();
  }

  return res;
}

int ffmpeg_adapter_converter::init_video(
  int input_video_width,
  int input_video_height,
  int input_video_format,
  int output_video_width,
  int output_video_height,
  int output_video_format) {

  int res = -1;

  if (input_video_width <= 0 ||
    input_video_height <= 0 ||
    input_video_format <= -1 ||
    output_video_width <= 0 ||
    output_video_height <= 0 ||
    output_video_format <= -1) {
    res = ErrorParametersInvalid;
    goto EXIT;
  }

  if (m_input_video_width == input_video_width &&
    m_input_video_height == input_video_height &&
    m_input_video_format == input_video_format &&
    m_output_video_width == output_video_width &&
    m_output_video_height == output_video_height &&
    m_output_video_format == output_video_format) {
    res = 0;
    goto EXIT;
  }

  destroy_video_convert_ctx();

  m_input_video_width = input_video_width;
  m_input_video_height = input_video_height;
  m_input_video_format = input_video_format;
  m_output_video_width = output_video_width;
  m_output_video_height = output_video_height;
  m_output_video_format = output_video_format;

  res = init_video_convert_ctx();
  if (res)
    goto EXIT;

  res = 0;
EXIT:
  if (res) {
    destroy_video_convert_ctx();
  }

  return res;
}

int ffmpeg_adapter_converter::get_output_audio_buffer_size() {
  int base = 1;
  switch (m_output_audio_format)
  {
  case AV_SAMPLE_FMT_U8:
    base = 1;
    break;
  case AV_SAMPLE_FMT_S16:
    base = 2;
    break;
  case AV_SAMPLE_FMT_S32:
    base = 4;
    break;
  default:
    break;
  }
  return m_output_audio_frame_size * m_output_audio_channels * base;
}

int ffmpeg_adapter_converter::get_audio_fifo_size() {
  if (m_p_audio_fifo)
    return av_audio_fifo_size(m_p_audio_fifo);
  else
    return 0;
}

int ffmpeg_adapter_converter::input_audio(AVFrame* p_new_frame) {
  int res = -1;
  int converted_samples = 0;

  if (!p_new_frame ||
    !m_p_sample_convert_ctx ||
    !m_p_audio_fifo ||
    !m_p_converted_input_samples ||
    !m_converted_input_samples_length) {
    res = ErrorNoContext;
    goto EXIT;
  }

  /** Convert the samples using the resampler. */
  converted_samples = swr_convert(m_p_sample_convert_ctx,
    m_p_converted_input_samples, m_converted_input_samples_length,
    (const uint8_t**)p_new_frame->data, p_new_frame->nb_samples);
  if (converted_samples < 0) {
    LOGE("Could not convert input samples (error '%s')\n", get_error_text(converted_samples));
    goto EXIT;
  }

  res = av_audio_fifo_realloc(m_p_audio_fifo, av_audio_fifo_size(m_p_audio_fifo) + converted_samples);
  if (res < 0) {
    LOGE("Could not reallocate FIFO\n");
    goto EXIT;
  }

  /** Store the new samples in the FIFO buffer. */
  res = av_audio_fifo_write(m_p_audio_fifo, (void**)m_p_converted_input_samples,
    converted_samples);
  if (res < 0 || res != converted_samples) {
    LOGE("Could not write data to FIFO\n");
    goto EXIT;
  }

  res = 0;
EXIT:
  if (res){

  }

  return res;
}

int ffmpeg_adapter_converter::output_audio(AVFrame** pp_new_frame, bool force) {
  int res = -1;
  int frame_size = 0;
  AVFrame* p_temp_frame = NULL;

  if (!pp_new_frame ||
    !m_p_sample_convert_ctx ||
    !m_p_audio_fifo ||
    !m_p_converted_input_samples ||
    !m_converted_input_samples_length) {
    res = ErrorNoContext;
    goto EXIT;
  }

  if (*pp_new_frame) {
    res = ErrorContextExistsAlready;
    goto EXIT;
  }

  /**
  * If we have enough samples for the encoder, we encode them.
  * At the end of the file, we pass the remaining samples to
  * the encoder.
  */
  if (av_audio_fifo_size(m_p_audio_fifo) >= m_output_audio_frame_size ||
    (force && av_audio_fifo_size(m_p_audio_fifo) > 0)) {

  } else {
    //done or buffer is empty
    res = ErrorBufferNotReady;
    goto EXIT;
  }

  /**
  * Take one frame worth of audio samples from the FIFO buffer,
  * encode it and write it to the output file.
  */

  /**
  * Use the maximum number of possible samples per frame.
  * If there is less than the maximum possible frame size in the FIFO
  * buffer use this number. Otherwise, use the maximum possible frame size
  */
  frame_size = FFMIN(av_audio_fifo_size(m_p_audio_fifo),
    m_output_audio_frame_size);

  /** Initialize temporary storage for one output frame. */
  p_temp_frame = ffmpeg_adapter_encoder::create_ffmpeg_audio_frame(frame_size,
    m_output_audio_format,
    m_output_audio_channels,
    av_get_default_channel_layout(m_output_audio_channels),
    m_output_audio_samplerate);
  if (!p_temp_frame) {
    res = ErrorAllocFailed;
    goto EXIT;
  }
  /**
  * Read as many samples from the FIFO buffer as required to fill the frame.
  * The samples are stored in the frame temporarily.
  */
  res = av_audio_fifo_read(m_p_audio_fifo, (void **)p_temp_frame->data, frame_size);
  if (res < 0) {
    LOGE("Could not read data from FIFO\n");
    goto EXIT;
  }

  *pp_new_frame = p_temp_frame;

  res = 0;
EXIT:
  if (res) {
  	if (NULL == p_temp_frame->buf[0] && NULL != p_temp_frame->data[0]){
		av_free(p_temp_frame->data[0]);
		p_temp_frame->data[0] = NULL;
	}
    RELEASE_FRAME(p_temp_frame);

    *pp_new_frame = NULL;
  }

  return res;
}

int ffmpeg_adapter_converter::convert_video(AVFrame** pp_new_frame, AVFrame* p_origin_frame) {
  int res = -1;
  int frame_size = 0;
  AVFrame* p_temp_frame = NULL;

  if (!pp_new_frame ||
    !p_origin_frame ||
    !m_p_img_convert_ctx) {
    res = ErrorNoContext;
    goto EXIT;
  }

  if (*pp_new_frame) {
    res = ErrorContextExistsAlready;
    goto EXIT;
  }

  // Allocate picture.
  if (!m_own_converted_video_frame) {
    p_temp_frame = ffmpeg_adapter_encoder::create_ffmpeg_video_frame(m_output_video_format, m_output_video_width,
      m_output_video_height);
    if (!p_temp_frame) {
      res = ErrorAllocFailed;
      goto EXIT;
    }

    //x264...
    p_temp_frame->pts = 0;
  } else {
    p_temp_frame = m_p_temp_frame;
  }

  if (USE_PERFORMANCE_OPTIMIZATION &&
    p_origin_frame->format == AV_PIX_FMT_BGR32 &&
    p_temp_frame->format == AV_PIX_FMT_YUV420P &&
    p_origin_frame->width == p_temp_frame->width &&
    p_origin_frame->height == p_temp_frame->height) {

    res = convert_rgba_to_yv12(
      p_origin_frame->data[0],
      p_temp_frame->data[0],
      p_temp_frame->width,
      p_temp_frame->height);
    if (res) {
      goto EXIT;
    }
  } else {
    res = sws_scale(m_p_img_convert_ctx, p_origin_frame->data, p_origin_frame->linesize, 0,
      p_origin_frame->height, p_temp_frame->data, p_temp_frame->linesize);
	if (p_temp_frame->data[0] != m_dst_data[0]){
		av_free(m_dst_data[0]);
		memcpy(m_dst_data, p_temp_frame->data, sizeof(p_temp_frame->data[0]) * 4);
	}
    if (res <= 0) {
      res = ErrorConvertError;
      goto EXIT;
    }
  }

  *pp_new_frame = p_temp_frame;

  res = 0;
EXIT:
  if (res) {
    if (p_temp_frame && p_temp_frame != m_p_temp_frame) {
      RELEASE_FRAME(p_temp_frame);
      p_temp_frame = NULL;
    }

    if (pp_new_frame) {
      *pp_new_frame = NULL;
    }
  }

  return res;
}

int ffmpeg_adapter_converter::is_converted_video_frame_owned() {
  return m_own_converted_video_frame;
}

void ffmpeg_adapter_converter::enable_own_converted_video_frame(int own) {
  m_own_converted_video_frame = own;
}

int ffmpeg_adapter_converter::convert_rgba_to_yv12(uint8_t* rgba_buffer, uint8_t* yuv_buffer, int width, int height) {
  int res = -1;

  int i = 0, j = 0;
  int h_offset = 0;
  int yuv_offset = 0;
  int rgb_offset = 0;
  int u_start = width * height;
  int v_start = u_start + (u_start >> 2);

  if (!rgba_buffer || !yuv_buffer || width <= 0 || height <= 0) {
    res = ErrorParametersInvalid;
    goto EXIT;
  }

  for (i = 0; i < u_start; ++i) {
    RgbToY(&(rgba_buffer[i << 2]), &(yuv_buffer[i]));
  }

  for (i = 0; i < height; i += 2) {
    for (j = 0; j < width; j += 2) {
      h_offset = i * width;

      rgb_offset = ((h_offset + j) << 2);
      yuv_offset = (h_offset >> 2) + (j >> 1);
      RgbToU(&(rgba_buffer[rgb_offset]), &(yuv_buffer[u_start + yuv_offset]));
      RgbToV(&(rgba_buffer[rgb_offset]), &(yuv_buffer[v_start + yuv_offset]));
    }
  }

  res = 0; //a hack~
EXIT:

  return res;
}

int ffmpeg_adapter_converter::convert_rgba_to_nv12(uint8_t* rgba_buffer, uint8_t* yuv_buffer, int width, int height) {
  int res = -1;

  int i = 0, j = 0;
  int h_offset = 0;
  int yuv_offset = 0;
  int rgb_offset = 0;
  int vu_start = width * height;

  if (!rgba_buffer || !yuv_buffer || width <= 0 || height <= 0) {
    res = ErrorParametersInvalid;
    goto EXIT;
  }

  for (i = 0; i < vu_start; ++i) {
    RgbToY(&(rgba_buffer[i << 2]), &(yuv_buffer[i]));
  }

  for (i = 0; i < height; i += 2) {
    for (j = 0; j < width; j += 2) {
      h_offset = i * width;

      rgb_offset = ((h_offset + j) << 2);
      yuv_offset = (h_offset >> 1) + j;
      RgbToV(&(rgba_buffer[rgb_offset]), &(yuv_buffer[vu_start + yuv_offset]));
      RgbToU(&(rgba_buffer[rgb_offset]), &(yuv_buffer[vu_start + yuv_offset + 1]));
    }
  }

  res = 0; //a hack~
EXIT:

  return res;
}

int ffmpeg_adapter_converter::convert_rgba_to_nv21(uint8_t* rgba_buffer, uint8_t* yuv_buffer, int width, int height) {
  int res = -1;

  int i = 0, j = 0;
  int h_offset = 0;
  int yuv_offset = 0;
  int rgb_offset = 0;
  int vu_start = width * height;

  if (!rgba_buffer || !yuv_buffer || width <= 0 || height <= 0) {
    res = ErrorParametersInvalid;
    goto EXIT;
  }

  for (i = 0; i < vu_start; ++i) {
    RgbToY(&(rgba_buffer[i << 2]), &(yuv_buffer[i]));
  }

  for (i = 0; i < height; i += 2) {
    for (j = 0; j < width; j += 2) {
      h_offset = i * width;

      rgb_offset = ((h_offset + j) << 2);
      yuv_offset = (h_offset >> 1) + j;
      RgbToU(&(rgba_buffer[rgb_offset]), &(yuv_buffer[vu_start + yuv_offset]));
      RgbToV(&(rgba_buffer[rgb_offset]), &(yuv_buffer[vu_start + yuv_offset + 1]));
    }
  }

  res = 0; //a hack~
EXIT:

  return res;
}

int ffmpeg_adapter_converter::convert_yv21_to_rgba(uint8_t* yuv_buffer, uint8_t* rgba_buffer, int width, int height) {
  int res = -1;
  int uv_linesize = 0;
  int uv_offset = 0;
  int u_start = 0;
  int v_start = 0;

  if (!yuv_buffer || !rgba_buffer || width <= 0 || height <= 0) {
    res = ErrorParametersInvalid;
    goto EXIT;
  }

  //yuv->rgba -- use java buffer
  u_start = width * height;
  v_start = u_start + (u_start >> 2);
  uv_linesize = width / 2;

  for (int uv_height_offset = 0, height_index = 0, index = 0; height_index < height; height_index++) {
    uv_height_offset = ((index / width) >> 1) * uv_linesize;

    for (int width_index = 0; width_index < width; width_index += 2, index += 2) {
      uv_offset = uv_height_offset + (width_index >> 1);

      YuvToRgba(yuv_buffer[index],
        yuv_buffer[u_start + uv_offset],
        yuv_buffer[v_start + uv_offset],
        (rgba_buffer + (index << 2)));

      YuvToRgba(yuv_buffer[index + 1],
        yuv_buffer[u_start + uv_offset],
        yuv_buffer[v_start + uv_offset],
        (rgba_buffer + ((index + 1) << 2)));
    }
  }

  res = 0;
EXIT:

  return res;
}

int ffmpeg_adapter_converter::init_audio_convert_ctx() {
  int res = -1;

  if (m_p_sample_convert_ctx ||
    m_p_audio_fifo ||
    m_p_converted_input_samples ||
    m_converted_input_samples_length) {
    res = ErrorDuplicatedInit;
    goto EXIT;
  }

  if (m_input_audio_samplerate <= -1 ||
    m_input_audio_format <= -1 ||
    m_input_audio_channels <= -1 ||
    m_output_audio_samplerate <= -1 ||
    m_output_audio_format <= -1 ||
    m_output_audio_channels <= -1 ||
    m_output_audio_frame_size <= 0) {
    res = ErrorParametersInvalid;
    goto EXIT;
  }

  m_p_sample_convert_ctx = swr_alloc();
  if (!m_p_sample_convert_ctx) {
    res = ErrorAllocFailed;
    goto EXIT;
  }

  av_opt_set_int(m_p_sample_convert_ctx, "in_sample_rate", m_input_audio_samplerate, 0);
  av_opt_set_int(m_p_sample_convert_ctx, "out_sample_rate", m_output_audio_samplerate, 0);
  av_opt_set_sample_fmt(m_p_sample_convert_ctx, "in_sample_fmt", (AVSampleFormat)m_input_audio_format, 0);
  av_opt_set_sample_fmt(m_p_sample_convert_ctx, "out_sample_fmt", (AVSampleFormat)m_output_audio_format, 0);
  av_opt_set_int(m_p_sample_convert_ctx, "in_channel_layout", av_get_default_channel_layout(m_input_audio_channels), 0);
  av_opt_set_int(m_p_sample_convert_ctx, "out_channel_layout", av_get_default_channel_layout(m_output_audio_channels), 0);

  /* initialize the resampling context */
  res = swr_init(m_p_sample_convert_ctx);
  if (res) {
    LOGE("Failed to initialize the resampling context\n");
    goto EXIT;
  }

  m_p_audio_fifo = av_audio_fifo_alloc((AVSampleFormat)m_output_audio_format, m_output_audio_channels, 1);
  if (!m_p_audio_fifo) {
    res = ErrorAllocFailed;
    goto EXIT;
  }

  m_converted_input_samples_length = MAX(m_output_audio_frame_size << 2, MIN_CONVERT_BUFFER_SIZE);
  res = av_samples_alloc_array_and_samples(&m_p_converted_input_samples, NULL,
    m_output_audio_channels,
    m_converted_input_samples_length,
    (AVSampleFormat)m_output_audio_format, 0);
  if (res < 0 || !m_p_converted_input_samples) {
    LOGE("Could not allocate converted input samples (error '%s')\n", get_error_text(res));
    res = ErrorAllocFailed;
    goto EXIT;
  }

  res = 0;
EXIT:
  if (res) {
    destroy_audio_convert_ctx();
  }

  return res;
}

void ffmpeg_adapter_converter::destroy_audio_convert_ctx() {
  if (m_p_sample_convert_ctx) {
    swr_free(&m_p_sample_convert_ctx);
    m_p_sample_convert_ctx = NULL;
  }

  if (m_p_audio_fifo) {
    av_audio_fifo_free(m_p_audio_fifo);
    m_p_audio_fifo = NULL;
  }

  if (m_p_converted_input_samples) {
    RELEASE_SAMPLE_PP_BUFFER(m_p_converted_input_samples);
    m_p_converted_input_samples = NULL;
  }

  m_converted_input_samples_length = 0;
}

int ffmpeg_adapter_converter::init_video_convert_ctx() {
  int res = -1;

  if (m_p_img_convert_ctx || m_p_temp_frame) {
    res = ErrorDuplicatedInit;
    goto EXIT;
  }

  if (m_input_video_width <= 0 ||
    m_input_video_height <= 0 ||
    m_input_video_format <= -1 ||
    m_output_video_width <= 0 ||
    m_output_video_height <= 0 ||
    m_output_video_format <= -1) {
    res = ErrorParametersInvalid;
    goto EXIT;
  }

  /* convert to the codec pix fmt/size if needed */
  m_p_img_convert_ctx = sws_getCachedContext(m_p_img_convert_ctx,
    m_input_video_width, m_input_video_height,
    (AVPixelFormat)m_input_video_format, m_output_video_width,
    m_output_video_height, (AVPixelFormat)m_output_video_format,
    SWS_BILINEAR, NULL, NULL, NULL);
  if (!m_p_img_convert_ctx) {
    res = ErrorAllocFailed;
    goto EXIT;
  }

  // Allocate picture.
  m_p_temp_frame = ffmpeg_adapter_encoder::create_ffmpeg_video_frame(m_output_video_format, m_output_video_width,
    m_output_video_height);
  if (!m_p_temp_frame) {
    res = ErrorAllocFailed;
    goto EXIT;
  }
  memcpy(m_dst_data, m_p_temp_frame->data, sizeof(m_dst_data[0]) * 4);

  m_p_temp_frame->pts = 0; //for x264

  res = 0;
EXIT:
  if (res) {
    destroy_video_convert_ctx();
  }

  return res;
}

void ffmpeg_adapter_converter::destroy_video_convert_ctx() {
	if (NULL != m_dst_data[0]){
		av_free(m_dst_data[0]);
		m_dst_data[0] = NULL;
	}

	if (m_p_temp_frame) {
		RELEASE_FRAME(m_p_temp_frame);
		m_p_temp_frame = NULL;
	}

	 if (m_p_img_convert_ctx) {
		 sws_freeContext(m_p_img_convert_ctx);
		 m_p_img_convert_ctx = NULL;
	}
}

