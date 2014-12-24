#include "ffmpeg_adapter_encoder.h"

#define MAX_AUDIO_BITRATE (64000) //if it's too large, the encoding and player will fail (on phone).
#define MAX_SAMPLERATE (48000) //same reason as above

#define ENABLE_DUMP 1
#define TAG ("ffmpeg_adapter_encoder")

#define CHECK_ALLOC_EXIT(value, res) if(NULL == (value)){ \
  res = ErrorAllocFailed; \
  goto EXIT;				\
}

//static long Total = 0;
//static long xxx = 0;

ffmpeg_adapter_encoder::ffmpeg_adapter_encoder() {
  m_p_file_path = NULL;

  m_p_video_extradata = NULL;
  m_video_extradata_length = 0;

  m_p_audio_extradata = NULL;
  m_audio_extradata_length = 0;

  m_p_fmt_ctx = NULL;

  m_p_converter = NULL;

  m_p_video_stream = NULL;
  m_p_audio_stream = NULL;

  m_video_codec_id = AV_CODEC_ID_H264;
  m_pix_fmt = AV_PIX_FMT_NONE;
  m_video_bitrate = 400000;
  m_frame_rate = 30.0;

  m_audio_codec_id = AV_CODEC_ID_NONE;
  m_sample_fmt = AV_SAMPLE_FMT_NONE; //AV_SAMPLE_FMT_S16
  m_audio_bitrate = 64000;
  m_samplerate = DEFAULT_AUDIO_SAMPLE_RATE;
  m_audio_channels = 2;

  m_video_quality = -1;
  m_audio_quality = -1;

  m_width = 0;
  m_height = 0;

  m_video_frame_count = 0;
  m_audio_frame_count = 0;

  m_enable_fake_audio = false;
  m_fake_audio_frame = NULL;

  m_started = 0;
}

ffmpeg_adapter_encoder::~ffmpeg_adapter_encoder() {
  end();
}

int ffmpeg_adapter_encoder::set_output_path(const char* path) {
  int res = -1;
  int length = 0;

  if (!path || strlen(path) <= 0) {
    res = ErrorParametersInvalid;
    goto EXIT;
  }

  if (m_p_file_path) {
    free(m_p_file_path);
    m_p_file_path = NULL;
  }

  length = strlen(path);
  m_p_file_path = (char*)calloc(1, length * sizeof(char)+1);
  CHECK_ALLOC_EXIT(m_p_file_path, res);

  strcpy(m_p_file_path, path);

  res = 0;
EXIT:
  if (res){
    if (m_p_file_path) {
      free(m_p_file_path);
      m_p_file_path = NULL;
    }
  }

  return res;
}

int ffmpeg_adapter_encoder::set_video_extra_data(uint8_t* extradata, long length) {
  int res = -1;
  int i = 0;
  uint8_t* sps = NULL;
  uint8_t* pps = NULL;
  int sps_length = 0;
  int pps_length = 0;
  int extradata_len = 0;
  int tmp = 0;
  int tmp2 = 0;

  if (!extradata || length <= 0) {
    res = ErrorParametersInvalid;
    goto EXIT;
  }

  if (is_started()) {
    res = ErrorContextExistsAlready;
    goto EXIT;
  }

  if (m_p_video_extradata) {
    free(m_p_video_extradata);
    m_p_video_extradata = NULL;
  }

  m_video_extradata_length = 0;

  for (i = 0; i < length - 4; ++i) {
    if (extradata[i] != 0x0 ||
      extradata[i + 1] != 0x0 ||
      extradata[i + 2] != 0x0 ||
      extradata[i + 3] != 0x1)
      continue;

    if (i + 5 >= length)
      break;

    if ((extradata[i + 4] & 0x0F) == 7) {
      sps = extradata + i;
    } else if ((extradata[i + 4] & 0x0F) == 8) {
      pps = extradata + i;
    }
  }

  if (!pps || !sps) {
    res = ErrorParametersInvalid;
    goto EXIT;
  }

  if (pps > sps) {
    sps_length = pps - sps;
    pps_length = extradata + length - pps;
  } else{
    pps_length = sps - pps;
    sps_length = extradata + length - sps;
  }

  extradata_len = 8 + sps_length - 4 + 1 + 2 + pps_length - 4;
  m_p_video_extradata = (uint8_t*)av_mallocz(extradata_len);
  CHECK_ALLOC_EXIT(m_p_video_extradata, res);

  m_video_extradata_length = extradata_len;
  m_p_video_extradata[0] = 0x01;
  m_p_video_extradata[1] = sps[4 + 1];
  m_p_video_extradata[2] = sps[4 + 2];
  m_p_video_extradata[3] = sps[4 + 3];
  m_p_video_extradata[4] = 0xFC | 3;
  m_p_video_extradata[5] = 0xE0 | 1;
  tmp = sps_length - 4;
  m_p_video_extradata[6] = (tmp >> 8) & 0x00ff;
  m_p_video_extradata[7] = tmp & 0x00ff;
  for (i = 0; i < tmp; i++)
    m_p_video_extradata[8 + i] = sps[4 + i];
  m_p_video_extradata[8 + tmp] = 0x01;
  tmp2 = pps_length - 4;
  m_p_video_extradata[8 + tmp + 1] = (tmp2 >> 8) & 0x00ff;
  m_p_video_extradata[8 + tmp + 2] = tmp2 & 0x00ff;
  for (i = 0; i < tmp2; i++)
    m_p_video_extradata[8 + tmp + 3 + i] = pps[4 + i];

  res = 0;
EXIT:
  if (res) {
    if (m_p_video_extradata) {
      av_free(m_p_video_extradata);
      m_p_video_extradata = NULL;
    }

    m_video_extradata_length = 0;
  }

  return res;
}

int ffmpeg_adapter_encoder::set_audio_extra_data(uint8_t* extradata, long length) {
  int res = -1;
  int i = 0;
  uint8_t* sps = NULL;
  uint8_t* pps = NULL;
  int sps_length = 0;
  int pps_length = 0;
  int extradata_len = 0;
  int tmp = 0;
  int tmp2 = 0;

  if (!extradata || length <= 0) {
    res = ErrorParametersInvalid;
    goto EXIT;
  }

  if (is_started()) {
    res = ErrorContextExistsAlready;
    goto EXIT;
  }

  if (m_p_audio_extradata) {
    free(m_p_audio_extradata);
    m_p_audio_extradata = NULL;
  }

  m_p_audio_extradata = (uint8_t*)av_mallocz(length);
  CHECK_ALLOC_EXIT(m_p_audio_extradata, res);

  m_audio_extradata_length = length;
  memcpy(m_p_audio_extradata, extradata, length);

  res = 0;
EXIT:
  if (res) {
    if (m_p_audio_extradata) {
      av_free(m_p_audio_extradata);
      m_p_audio_extradata = NULL;
    }

    m_audio_extradata_length = 0;
  }

  return res;
}

int ffmpeg_adapter_encoder::set_resolution(int width, int height) {
  int res = -1;

  if (width <= 0 || height <= 0) {
    res = ErrorParametersInvalid;
    goto EXIT;
  }

  if ((m_width % 2) != 0 || (m_height % 2) != 0) {
    res = ErrorParametersInvalid;
    goto EXIT;
  }

  m_width = width;
  m_height = height;

  res = 0;
EXIT:
  return res;
}

int ffmpeg_adapter_encoder::set_video_quality(int quality) {
  m_video_quality = (double)quality;
  return 0;
}

int ffmpeg_adapter_encoder::set_fps(double fps) {
  int res = -1;

  if (fps <= 0) {
    res = ErrorParametersInvalid;
    goto EXIT;
  }

  m_frame_rate = fps;

  res = 0;
EXIT:
  return res;
}

int ffmpeg_adapter_encoder::set_audio_channels(int channels) {
  int res = -1;

  if (channels <= 0 || channels > 2) {
    res = ErrorParametersInvalid;
    goto EXIT;
  }

  m_audio_channels = channels;

  res = 0;
EXIT:
  return res;
}

int ffmpeg_adapter_encoder::set_sample_format(int sample_fmt) {
  int res = -1;

  if (sample_fmt <= 0) {
    res = ErrorParametersInvalid;
    goto EXIT;
  }

  //TODO -- need to check the value
  m_sample_fmt = (AVSampleFormat)sample_fmt;

  res = 0;
EXIT:
  return res;
}

int ffmpeg_adapter_encoder::set_samplerate(int samplerate){
  int res = -1;

  if (samplerate <= 0) {
    res = ErrorParametersInvalid;
    goto EXIT;
  }

  samplerate = samplerate > MAX_SAMPLERATE ? MAX_SAMPLERATE : samplerate;

  m_samplerate = samplerate;

  res = 0;
EXIT:
  return res;
}

int ffmpeg_adapter_encoder::set_audio_bitrate(int bitrate) {
  int res = -1;

  if (bitrate < 0) {
    res = ErrorParametersInvalid;
    goto EXIT;
  }

  if (bitrate == 0) {
    res = 0;
    goto EXIT;
  }

  bitrate = bitrate > MAX_AUDIO_BITRATE ? MAX_AUDIO_BITRATE : bitrate;

  m_audio_bitrate = bitrate;

  res = 0;
EXIT:
  return res;
}

int ffmpeg_adapter_encoder::enable_fake_audio(bool enable) {
  int res = -1;

  //release every time when this function be called
  if (m_fake_audio_frame) {
    RELEASE_FRAME(m_fake_audio_frame);
    m_fake_audio_frame = NULL;
  }

  m_enable_fake_audio = enable;

  res = 0;
EXIT:
  return res;
}

int ffmpeg_adapter_encoder::use_fake_audio() {
  return m_enable_fake_audio;
}

int ffmpeg_adapter_encoder::is_started() {
  return m_started;
}

int ffmpeg_adapter_encoder::start() {
  int res = -1;

  LOGD("%s %d E. ", __func__, __LINE__);

  if (m_p_converter || m_p_audio_stream || m_p_video_stream || m_p_fmt_ctx) {
    res = ErrorDuplicatedInit;
    goto EXIT;
  }

  if (!m_p_file_path || strlen(m_p_file_path) <= 0) {
    res = ErrorParametersInvalid;
    goto EXIT;
  }

  // Initialize libavcodec
  av_register_all();
  avcodec_register_all();

  // allocate context
  m_p_fmt_ctx = avformat_alloc_context();
  CHECK_ALLOC_EXIT(m_p_fmt_ctx, res);

  // Create format as (container = auto)
  m_p_fmt_ctx->oformat = av_guess_format(NULL, m_p_file_path, NULL);
  CHECK_ALLOC_EXIT(m_p_fmt_ctx->oformat, res);

  strncpy(m_p_fmt_ctx->filename, m_p_file_path,
    MIN(strlen(m_p_file_path), sizeof(m_p_fmt_ctx->filename)));

  if (m_video_codec_id != AV_CODEC_ID_NONE) {
    m_p_fmt_ctx->oformat->video_codec = m_video_codec_id;
  } else if (!strcmp(m_p_fmt_ctx->oformat->extensions, "flv")) {
    m_p_fmt_ctx->oformat->video_codec = AV_CODEC_ID_FLV1;
  } else if (!strcmp(m_p_fmt_ctx->oformat->extensions, "mp4")) {
    m_p_fmt_ctx->oformat->video_codec = AV_CODEC_ID_MPEG4;
  } else if (!strcmp(m_p_fmt_ctx->oformat->extensions, "3gp")) {
    m_p_fmt_ctx->oformat->video_codec = AV_CODEC_ID_H263;
  } else if (!strcmp(m_p_fmt_ctx->oformat->extensions, "avi")) {
    m_p_fmt_ctx->oformat->video_codec = AV_CODEC_ID_HUFFYUV;
  }

  if (m_audio_codec_id != AV_CODEC_ID_NONE) {
    m_p_fmt_ctx->oformat->audio_codec = m_audio_codec_id;
  } else if (!strcmp(m_p_fmt_ctx->oformat->extensions, "flv")
    || !strcmp(m_p_fmt_ctx->oformat->extensions, "mp4")
    || !strcmp(m_p_fmt_ctx->oformat->extensions, "3gp")) {
    m_p_fmt_ctx->oformat->audio_codec = AV_CODEC_ID_AAC;
  } else if (!strcmp(m_p_fmt_ctx->oformat->extensions, "avi")) {
    m_p_fmt_ctx->oformat->audio_codec = AV_CODEC_ID_PCM_S16LE;
  }

  if (m_p_fmt_ctx->oformat->video_codec) {
    if (use_3rd_video_encoder()) {
      res = open_video_stream(m_p_video_extradata, m_video_extradata_length);
    } else {
      res = open_video_stream();
    }
    if (res) {
      goto EXIT;
    }
  }

  if (m_p_fmt_ctx->oformat->audio_codec) {
    if (use_3rd_audio_encoder()) {
      res = open_audio_stream(m_p_audio_extradata, m_audio_extradata_length);
    } else {
      res = open_audio_stream();
    }
    if (res) {
      goto EXIT;
    }
  }

  // Open Video and Audio stream
  if (!m_p_video_stream && !m_p_audio_stream) {
    //if both video and audio failed, then error
    res = ErrorNoStreamFound;
    goto EXIT;
  }

#ifdef ENABLE_DUMP
  av_dump_format(m_p_fmt_ctx, 0, m_p_file_path, 1);
#endif //ENABLE_DUMP

  if (!(m_p_fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
    res = avio_open(&m_p_fmt_ctx->pb, m_p_file_path, AVIO_FLAG_WRITE);
    if (res < 0)
      goto EXIT;
  }

  m_p_converter = new ffmpeg_adapter_converter();
  CHECK_ALLOC_EXIT(m_p_converter, res);

  res = avformat_write_header(m_p_fmt_ctx, NULL);
  CHK_RES(res);

EXIT:
  if (res) {
    release();
  } else {
    m_started = 1;
  }

  LOGD("%s %d X res =%d ", __func__, __LINE__, res);

  return res;
}

int ffmpeg_adapter_encoder::check_start() {
  if (!is_started()) {
    return start();
  } else {
    return 0;
  }
}

int ffmpeg_adapter_encoder::use_3rd_video_encoder() {
  return (m_p_video_extradata != NULL);
}

int ffmpeg_adapter_encoder::use_3rd_audio_encoder() {
  return (m_p_audio_extradata != NULL);
}

void ffmpeg_adapter_encoder::end() {
  m_started = 0;

  if (m_p_fmt_ctx) {
    av_write_trailer(m_p_fmt_ctx);
    release();
  }
}

int ffmpeg_adapter_encoder::add_video_frame(AVFrame* p_new_frame) {
  return add_video_frame(p_new_frame, 0);
}

int ffmpeg_adapter_encoder::add_video_frame(AVFrame* p_new_frame, long timestamp) {
  int res = -1;
  AVPacket video_pkt = { 0 };
  AVCodecContext* p_video_c = NULL;
  AVFrame* p_target_frame = NULL;
  int got_video_packet = 0;

  LOGD("%s%d E, frame=%p", __func__, __LINE__, p_new_frame);

  res = check_start();
  CHK_RES(res);

  if (!m_p_fmt_ctx) {
    res = ErrorNoContext;
    goto EXIT;
  }

  if (!m_p_video_stream || !m_p_converter || !m_p_video_stream->codec) {
    res = ErrorNoStreamFound;
    goto EXIT;
  }

  p_video_c = m_p_video_stream->codec;

  if (p_new_frame && !p_new_frame->data[0]) {
    res = ErrorParametersInvalid;
    goto EXIT;
  }

  if (p_new_frame) {
    res = m_p_converter->init_video(
      p_new_frame->width,
      p_new_frame->height,
      p_new_frame->format,
      p_video_c->width,
      p_video_c->height,
      p_video_c->pix_fmt);
    if (res) {
      goto EXIT;
    }

    res = m_p_converter->convert_video(&p_target_frame, p_new_frame);
    CHK_RES(res);
  } else {
    //it's common case -- in the end of processing, will be here to flush rest frames into files.
    if (use_3rd_video_encoder()) {
      res = 0;
      goto EXIT;
    }
  }

  if (m_p_fmt_ctx->oformat->flags & AVFMT_RAWPICTURE) {
    /* raw video case. The API may change slightly in the future for that? */
    av_init_packet(&video_pkt);
    video_pkt.flags |= AV_PKT_FLAG_KEY;
    video_pkt.stream_index = m_p_video_stream->index;
    video_pkt.data = p_target_frame->data[0];
    video_pkt.size = sizeof(AVPicture);
  } else {
    /* encode the image */
    av_init_packet(&video_pkt);
    video_pkt.data = NULL;
    video_pkt.size = 0;

    if (p_target_frame) {
      p_target_frame->quality = p_video_c->global_quality;

      if (timestamp != 0) {
        p_target_frame->pts = timestamp;
      } else {
        //  magic required by libx264
        p_target_frame->pts = m_video_frame_count;
      }
    }

    //temporary looping... for libx264
    while (!got_video_packet) {
      res = avcodec_encode_video2(p_video_c, &video_pkt, p_target_frame,
        &got_video_packet);
      CHK_RES(res);

      if (!p_target_frame) {
        break;
      }
    }

    /* if zero size, it means the image was buffered */
    if (got_video_packet) {
      if (video_pkt.pts != AV_NOPTS_VALUE) {
        video_pkt.pts = av_rescale_q(video_pkt.pts, p_video_c->time_base,
          m_p_video_stream->time_base);
      }
      if (video_pkt.dts != AV_NOPTS_VALUE) {
        video_pkt.dts = av_rescale_q(video_pkt.dts, p_video_c->time_base,
          m_p_video_stream->time_base);
      }

      video_pkt.stream_index = m_p_video_stream->index;
    } else {
      //todo
      goto FRAME_;
    }
  }

  if (!m_p_audio_stream) {
    res = av_write_frame(m_p_fmt_ctx, &video_pkt);
    CHK_RES(res);
  } else {
    res = av_interleaved_write_frame(m_p_fmt_ctx, &video_pkt);
    CHK_RES(res);
  }

FRAME_:
  m_video_frame_count++;

EXIT:
  if (m_p_converter && !m_p_converter->is_converted_video_frame_owned()) {
    if (p_target_frame) {
      RELEASE_FRAME(p_target_frame);
      p_target_frame = NULL;
    }
  }

  av_free_packet(&video_pkt);

  LOGD("%s%d X, frame_count = %d res=%d", __func__, __LINE__, m_video_frame_count, res);

  return res;
}

int ffmpeg_adapter_encoder::add_compressed_video_frame(uint8_t* buffer, long length, long timestamp, int flag) {
  int res = -1;
  int size = 0;
  AVPacket video_pkt = { 0 };

  LOGD("%s%d E, buffer length=%ld timestamp=%ld", __func__, __LINE__, length, timestamp);

  if (!buffer || length <= 0) {
    res = ErrorParametersInvalid;
    goto EXIT;
  }

  res = check_start();
  CHK_RES(res);

  res = av_new_packet(&video_pkt, length);
  CHK_RES(res);

  memcpy(video_pkt.data, buffer, length * sizeof(uint8_t));

  size = length - 4;
  video_pkt.data[1] = (size >> 16) & 0x00ff;
  video_pkt.data[2] = (size >> 8) & 0x00ff;
  video_pkt.data[3] = size & 0x00ff;

  if (flag) {
    video_pkt.flags |= AV_PKT_FLAG_KEY;
  }
  video_pkt.stream_index = m_p_video_stream->index;

  video_pkt.dts = AV_NOPTS_VALUE;
  video_pkt.pts = AV_NOPTS_VALUE;

  if (!m_p_audio_stream) {
    res = av_write_frame(m_p_fmt_ctx, &video_pkt);
    CHK_RES(res);
  } else {
    res = av_interleaved_write_frame(m_p_fmt_ctx, &video_pkt);
    CHK_RES(res);
  }

  m_video_frame_count++;

  res = 0;
EXIT:
  if (res){

  }

  //the buffer ownership ...
  av_free_packet(&video_pkt);

  LOGD("%s%d X, frame_count = %d res=%d", __func__, __LINE__, m_video_frame_count, res);

  return res;
}

int ffmpeg_adapter_encoder::add_audio_frame(AVFrame* p_new_frame) {
  return add_audio_frame(p_new_frame, 0);
}

int ffmpeg_adapter_encoder::add_audio_frame(AVFrame* p_new_frame, long timestamp) {
  int res = -1;
  AVPacket audio_pkt = { 0 };
  int got_output = 0;
  bool finished = false;
  int done = 0;
  AVFrame *output_frame = NULL;
  int converted_length = 0;

  LOGD("%s %d frame=%p E ", __func__, __LINE__, p_new_frame);

  res = check_start();
  CHK_RES(res);

  if (!m_p_fmt_ctx) {
    res = ErrorNoContext;
    goto EXIT;
  }

  if (!m_p_audio_stream || !m_p_audio_stream->codec || !m_p_converter) {
    res = ErrorNoEncoderFound;
    goto EXIT;
  }

  if (!p_new_frame) {
    if (m_p_converter->get_audio_fifo_size() > 0) {
      //in this case, it's finished
      finished = true;
    } else if (m_enable_fake_audio) {
      if (!m_fake_audio_frame) {
        //What??? it means there is no any audio exists before
        m_fake_audio_frame = create_ffmpeg_audio_frame(m_p_audio_stream->codec->frame_size,
          m_p_audio_stream->codec->sample_fmt,
          m_p_audio_stream->codec->channels,
          m_p_audio_stream->codec->channel_layout,
          m_p_audio_stream->codec->sample_rate);
        CHECK_ALLOC_EXIT(m_fake_audio_frame, res);
      }

      p_new_frame = m_fake_audio_frame;
    } else {
      //no need to do more
      res = 0;
      goto EXIT;
    }
  } else {
    LOGD("%s %d HERE sample=%d, channels=%d, format=%d, size=%d ", __func__, __LINE__,
      p_new_frame->sample_rate,
      p_new_frame->channels,
      p_new_frame->format,
      p_new_frame->linesize[0]);
  }

  if (p_new_frame) {
    res = m_p_converter->init_audio(p_new_frame->sample_rate,
      p_new_frame->channels,
      p_new_frame->format,
      m_p_audio_stream->codec->sample_rate,
      m_p_audio_stream->codec->channels,
      m_p_audio_stream->codec->sample_fmt,
      m_p_audio_stream->codec->frame_size);
    CHK_RES(res);

    res = m_p_converter->input_audio(p_new_frame);
    CHK_RES(res);
  }

  do
  {
    res = m_p_converter->output_audio(&output_frame, finished);
    if (res == ErrorBufferNotReady) {
      //wait for next
      res = 0;
      break;
    } else if ((res || !output_frame) && !finished) {
      //error occurred
      break;
    }

    res = encode_audio_frame(output_frame, m_p_fmt_ctx, m_p_audio_stream, m_p_video_stream, &got_output);
    CHK_RES(res);
	if (NULL == output_frame->buf[0] && NULL != output_frame->data[0])
		av_free(output_frame->data[0]);
	output_frame->data[0] = NULL;
    av_frame_free(&output_frame);
  } while (got_output);

  CHK_RES(res);

  res = 0;
EXIT:
  if (res) {

  }

  if (output_frame) {
  	if (NULL == output_frame->buf[0] && NULL != output_frame->data[0])
		av_free(output_frame->data[0]);
    av_frame_free(&output_frame);
  }

  LOGD("%s %d res=%d X ", __func__, __LINE__, res);

  return res;
}

int ffmpeg_adapter_encoder::add_compressed_audio_frame(uint8_t* buffer, long length, long timestamp) {
  int res = -1;
  int size = 0;
  AVPacket audio_pkt = { 0 };

  LOGD("%s%d E, buffer length=%ld timestamp=%ld", __func__, __LINE__, length, timestamp);

  if (!buffer || length <= 0) {
    res = ErrorParametersInvalid;
    goto EXIT;
  }

  res = check_start();
  CHK_RES(res);

  res = av_new_packet(&audio_pkt, length);
  CHK_RES(res);

  memcpy(audio_pkt.data, buffer, length * sizeof(uint8_t));

  audio_pkt.stream_index = m_p_audio_stream->index;
  audio_pkt.flags |= AV_PKT_FLAG_KEY;

  audio_pkt.dts = m_audio_frame_count * 1024;
  audio_pkt.pts = audio_pkt.dts;

  if (!m_p_video_stream) {
    res = av_write_frame(m_p_fmt_ctx, &audio_pkt);
    CHK_RES(res);
  } else {
    res = av_interleaved_write_frame(m_p_fmt_ctx, &audio_pkt);
    CHK_RES(res);
  }
  m_audio_frame_count++;
  res = 0;
EXIT:
  if (res){

  }

  //the buffer ownership ...
  av_free_packet(&audio_pkt);

  LOGD("%s%d X, res=%d", __func__, __LINE__, res);

  return res;
}

int ffmpeg_adapter_encoder::get_fps() {
  return (int)m_frame_rate;
}

long ffmpeg_adapter_encoder::get_current_video_duration() {
  if (m_frame_rate) {
    return static_cast<long>(
      (double)m_video_frame_count /
      (double)m_frame_rate *
      1000000L);
  } else {
    return 0;
  }
}

long ffmpeg_adapter_encoder::get_current_audio_duration() {
  if (!m_p_audio_stream || !m_p_audio_stream->time_base.den)
    return -1;
  if (m_p_audio_stream->pts.val != 0){
    return static_cast<long>(
      (double)(m_p_audio_stream->pts.val * m_p_audio_stream->time_base.num) /
      (double)m_p_audio_stream->time_base.den *
      1000000L);
  } else
  {
    return static_cast<long>(
      (double)(m_audio_frame_count * 1024 * m_p_audio_stream->time_base.num) /
      (double)m_p_audio_stream->time_base.den *
      1000000L);
  }

  return static_cast<long>(m_p_audio_stream->duration);
}

void ffmpeg_adapter_encoder::add_image_file(const char* file_name, int width,
  int height, int timestamp) {
  AVCodecContext *p_codec_ctx = NULL;
  AVFormatContext *p_fmt_ctx = NULL;
  AVFrame *p_frame = NULL;
  AVCodec *p_codec = NULL;
  int res = 0;
  int finished = false;
  int size = 0;
  uint8_t *buffer = NULL;
  AVPacket packet = { 0 };
  int frame_number = 0;

  res = avformat_open_input(&p_fmt_ctx, file_name, NULL, NULL);
  CHK_RES(res);

  p_codec_ctx = p_fmt_ctx->streams[0]->codec;
  p_codec_ctx->width = width;
  p_codec_ctx->height = height;
  p_codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;

  // Find the decoder for the video stream
  p_codec = avcodec_find_decoder(p_codec_ctx->codec_id);
  CHECK_ALLOC_EXIT(p_codec, res);

  // Open codec
  res = avcodec_open2(p_codec_ctx, p_codec, NULL);
  CHK_RES(res);

  p_frame = av_frame_alloc();
  CHECK_ALLOC_EXIT(p_frame, res);

  // Determine required buffer size and allocate buffer
  size = avpicture_get_size(AV_PIX_FMT_YUV420P, p_codec_ctx->width,
    p_codec_ctx->height);

  buffer = (uint8_t *)av_malloc(size * sizeof(uint8_t));

  avpicture_fill((AVPicture *)p_frame, buffer, AV_PIX_FMT_YUV420P,
    p_codec_ctx->width, p_codec_ctx->height);

  while (av_read_frame(p_fmt_ctx, &packet) >= 0) {
    if (packet.stream_index != 0)
      continue;

    res = avcodec_decode_video2(p_codec_ctx, p_frame, &finished,
      &packet);
    if (res > 0) {
      p_frame->quality = 4;
      //return pFrame;
    } else {
      LOGE("Error [%d] while decoding frame: %s\n", res,
        strerror(AVERROR(res)));
    }
  }

  if (p_frame->format == AV_PIX_FMT_YUVJ420P) {
    p_frame->format = AV_PIX_FMT_YUV420P;
  }

  res = add_video_frame(p_frame, timestamp);
  CHK_RES(res);

  res = 0;
EXIT:
  if (res) {
  }

  if (p_frame) {
    RELEASE_FRAME(p_frame);
    p_frame = NULL;
  }

  if (buffer) {
    av_free(buffer);
    buffer = NULL;
  }

  if (p_codec_ctx) {
    avcodec_close(p_codec_ctx);
  }

  if (p_fmt_ctx) {
    avformat_close_input(&p_fmt_ctx);
  }

  return;
}

void ffmpeg_adapter_encoder::release() {
  if (m_p_file_path) {
    free(m_p_file_path);
    m_p_file_path = NULL;
  }

  if (m_fake_audio_frame) {
    RELEASE_FRAME(m_fake_audio_frame);
    m_fake_audio_frame = NULL;
  }

  if (m_p_converter) {
    delete m_p_converter;
    m_p_converter = NULL;
  }

  if (m_p_fmt_ctx) {
    if (m_p_video_stream && m_p_video_stream->codec) {
      avcodec_close(m_p_video_stream->codec);
      //m_p_video_stream->codec = NULL; //FFMPEG BUG...
    }

    if (m_p_audio_stream && m_p_audio_stream->codec) {
      avcodec_close(m_p_audio_stream->codec);
      //m_p_audio_stream->codec = NULL; //FFMPEG BUG...
    }

    avio_close(m_p_fmt_ctx->pb);
    avformat_free_context(m_p_fmt_ctx);
    m_p_fmt_ctx = NULL;
    m_p_video_stream = NULL;
    m_p_audio_stream = NULL;
  }

  if (m_p_video_extradata) {
    av_free(m_p_video_extradata);
    m_p_video_extradata = NULL;
  }

  m_video_extradata_length = 0;

  if (m_p_audio_extradata) {
    av_free(m_p_audio_extradata);
    m_p_audio_extradata = NULL;
  }

  m_audio_extradata_length = 0;
}

AVFrame* ffmpeg_adapter_encoder::create_ffmpeg_audio_frame(int nb_samples, int format, int channels, int channel_layout, int sample_rate) {
  int res = -1;
  AVFrame *frame = NULL;
  uint8_t *frame_buf = NULL;
  int size;

  LOGD("%s %d E. nb_sample=%d, format=%d channels=%d channel_layout=%d sample_rate=%d", __func__, __LINE__, nb_samples, format, channels, channel_layout, sample_rate);

  /* frame containing input raw audio */
  frame = av_frame_alloc();
  CHECK_ALLOC_EXIT(frame, res);

  frame->nb_samples = nb_samples;
  frame->format = format;
  frame->channel_layout = channel_layout;
  frame->channels = channels;
  frame->sample_rate = sample_rate;

  /* the codec gives us the frame size, in samples,
  * we calculate the size of the samples buffer in bytes */
  size = av_samples_get_buffer_size(NULL, frame->channels, frame->nb_samples,
    (AVSampleFormat)frame->format, 0);
  frame_buf = (uint8_t*)av_malloc(size);
  CHECK_ALLOC_EXIT(frame_buf, res);

  memset(frame_buf, 0, size);

  /* setup the data pointers in the AVFrame */
  res = avcodec_fill_audio_frame(frame, frame->channels, (AVSampleFormat)frame->format,
    (const uint8_t*)frame_buf, size, 0);
  if (res < 0) {
    goto EXIT;
  }

  res = 0;
EXIT:
  if (res) {
    if (frame) {
      RELEASE_FRAME(frame);
      frame = NULL;
    }
	av_free(frame_buf);
  }

  LOGD("%s %d X. res=%d", __func__, __LINE__, res);
  return frame;
}

AVFrame* ffmpeg_adapter_encoder::create_ffmpeg_audio_frame(int nb_samples, int format, int channels, int channel_layout, int sample_rate, uint8_t* buffer, int buffer_length) {
  int res = -1;
  AVFrame *frame = NULL;
  uint8_t *frame_buf = NULL;
  int size;

  LOGD("%s %d E. nb_sample=%d, format=%d channels=%d channel_layout=%d sample_rate=%d buffer=%p buffer_length=%d", __func__, __LINE__, nb_samples, format, channels, channel_layout, sample_rate, buffer, buffer_length);

  /* frame containing input raw audio */
  frame = av_frame_alloc();
  CHECK_ALLOC_EXIT(frame, res);

  frame->nb_samples = nb_samples;
  frame->format = format;
  frame->channel_layout = channel_layout;
  frame->channels = channels;
  frame->sample_rate = sample_rate;
  frame->linesize[0] = buffer_length;
  frame->data[0] = buffer;

  ///* the codec gives us the frame size, in samples,
  //* we calculate the size of the samples buffer in bytes */
  //size = av_samples_get_buffer_size(NULL, frame->channels, frame->nb_samples,
  //  (AVSampleFormat)frame->format, 0);

  //if (size < buffer_length) {
  //  res = ErrorParametersInvalid;
  //  goto EXIT;
  //} else if (size == buffer_length) {
  //  frame_buf = buffer;
  //} else {
  //  frame_buf = (uint8_t*)av_malloc(size);
  //  memset(frame_buf, 0, size);
  //  memcpy(frame_buf, buffer, buffer_length * sizeof(uint8_t));
  //}

  //if (!frame_buf) {
  //  res = ErrorAllocFailed;
  //  goto EXIT;
  //}

  ///* setup the data pointers in the AVFrame */
  //res = avcodec_fill_audio_frame(frame, frame->channels, (AVSampleFormat)frame->format,
  //  (const uint8_t*)frame_buf, size, 0);
  //if (res < 0) {
  //  goto EXIT;
  //}

  res = 0;
EXIT:
  if (res) {
    if (frame) {
      RELEASE_FRAME(frame);
      frame = NULL;
    }
  }

  if (frame_buf) {
    if (frame_buf != buffer) {
      av_free(frame_buf);
      frame_buf = NULL;
    }
  }

  LOGD("%s %d X. res=%d", __func__, __LINE__, res);
  return frame;
}

/**
* Initialize one input frame for writing to the output file.
* The frame will be exactly frame_size samples large.
*/
int ffmpeg_adapter_encoder::create_ffmpeg_audio_frame(AVFrame **frame,
  AVCodecContext *output_codec_context,
  int frame_size)
{
  int error;
  /** Create a new frame to store the audio samples. */
  if (!(*frame = av_frame_alloc())) {
    LOGE("Could not allocate output frame\n");
    return AVERROR_EXIT;
  }
  /**
  * Set the frame's parameters, especially its size and format.
  * av_frame_get_buffer needs this to allocate memory for the
  * audio samples of the frame.
  * Default channel layouts based on the number of channels
  * are assumed for simplicity.
  */
  (*frame)->nb_samples = frame_size;
  (*frame)->channel_layout = output_codec_context->channel_layout;
  (*frame)->format = output_codec_context->sample_fmt;
  (*frame)->sample_rate = output_codec_context->sample_rate;
  /**
  * Allocate the samples of the created frame. This call will make
  * sure that the audio frame can hold as many samples as specified.
  */
  if ((error = av_frame_get_buffer(*frame, 0)) < 0) {
    LOGE("Could allocate output frame samples (error '%s')\n", get_error_text(error));
    av_frame_free(frame);
    return error;
  }

  return 0;
}

/** Encode one frame worth of audio to the output file. */
int ffmpeg_adapter_encoder::encode_audio_frame(AVFrame *frame,
  AVFormatContext *output_format_context,
  AVStream* output_audio_stream,
  bool interval,
  int *data_present)
{
  /** Packet used for temporary storage. */
  AVPacket output_packet = { 0 };
  int res = -1;
  av_init_packet(&output_packet);
  output_packet.data = NULL;
  output_packet.size = 0;

  if (!output_format_context || !output_audio_stream->codec || !data_present) {
    res = ErrorParametersInvalid;
    goto EXIT;
  }

  /**
  * Encode the audio frame and store it in the temporary packet.
  * The output audio stream encoder is used to do this.
  */
  //xxx = system_timestamp();
  res = avcodec_encode_audio2(output_audio_stream->codec, &output_packet,
    frame, data_present);
  //Total += system_timestamp() - xxx;
  //LOGE("Encode audio code ---- %ld res=%d", Total, res);
  if (res < 0) {
    LOGE("Could not encode frame (error '%s')\n", get_error_text(res));
    goto EXIT;
  }

  if (frame) {
    frame->quality = output_audio_stream->codec->global_quality;
  }

  /** Write one audio frame from the temporary packet to the output file. */
  if (*data_present) {
    if (interval) {
      if (frame) {
        if (output_packet.pts != AV_NOPTS_VALUE) {
          output_packet.pts = av_rescale_q(output_packet.pts, output_audio_stream->codec->time_base, output_audio_stream->time_base);
        }
        if (output_packet.dts != AV_NOPTS_VALUE) {
          output_packet.dts = av_rescale_q(output_packet.dts, output_audio_stream->codec->time_base, output_audio_stream->time_base);
        }
      }

      output_packet.stream_index = output_audio_stream->index;
      output_packet.flags |= AV_PKT_FLAG_KEY;

      res = av_interleaved_write_frame(output_format_context, &output_packet);
      if (res < 0){
        LOGE("Could not write frame (error '%s')\n", get_error_text(res));
        goto EXIT;
      }
    } else {
      res = av_write_frame(output_format_context, &output_packet);
      if (res < 0) {
        LOGE("Could not write frame (error '%s')\n", get_error_text(res));
        goto EXIT;
      }
    }
  }

EXIT:
  av_free_packet(&output_packet);

  return res;
}

AVFrame* ffmpeg_adapter_encoder::create_ffmpeg_video_frame(int pix_fmt, int width,
  int height) {
  int res = -1;
  AVFrame *frame = NULL;
  uint8_t *frame_buf = NULL;
  int size;

  LOGD("%s %d E pix_fmt=%d width=%d  height=%d", __func__, __LINE__, pix_fmt,
    width, height);

  if (pix_fmt < 0 || width <= 0 || height <= 0) {
    res = ErrorParametersInvalid;
    goto EXIT;
  }

  frame = av_frame_alloc();
  CHECK_ALLOC_EXIT(frame, res);

  size = avpicture_get_size((AVPixelFormat)pix_fmt, width, height);

  frame_buf = (uint8_t *)av_malloc(size);
  CHECK_ALLOC_EXIT(frame_buf, res);

  res = avpicture_fill((AVPicture *)frame, frame_buf, (AVPixelFormat)pix_fmt,
    width, height);
  if (res < 0) {
    goto EXIT;
  }

  //re-fill again.
  frame->width = width;
  frame->height = height;
  frame->format = pix_fmt;
  //frame->data[0] = frame_buf;
  frame->pts = 0; // magic required by libx264

  res = 0;

EXIT:
  if (res) {
    if (frame) {
      RELEASE_FRAME(frame);
      frame = NULL;
    }
  }

  LOGD("%s %d X. res=%d", __func__, __LINE__, res);
  return frame;
}

AVFrame* ffmpeg_adapter_encoder::create_ffmpeg_video_frame(int pix_fmt,
  uint8_t* buffer, int width, int height) {
  int res = -1;
  AVFrame *frame = NULL;

  LOGD("%s %d E pix_fmt=%d width=%d  height=%d", __func__, __LINE__, pix_fmt,
    width, height);

  if (pix_fmt < 0 || !buffer || width <= 0 || height <= 0) {
    res = ErrorParametersInvalid;
    goto EXIT;
  }

  frame = av_frame_alloc();
  CHECK_ALLOC_EXIT(frame, res);

  res = avpicture_fill((AVPicture *)frame, buffer, (AVPixelFormat)pix_fmt,
    width, height);
  if (res < 0) {
    goto EXIT;
  }

  //re-fill again.
  frame->width = width;
  frame->height = height;
  frame->format = pix_fmt;
  frame->data[0] = buffer;
  frame->pts = 0; // magic required by libx264

  res = 0;

EXIT:
  if (res) {
    if (frame) {
      RELEASE_FRAME(frame);
      frame = NULL;
    }
  }

  LOGD("%s %d X. res=%d", __func__, __LINE__, res);
  return frame;
}

int ffmpeg_adapter_encoder::open_video_stream() {
  int res = -1;
  AVCodec* p_video_codec = NULL;
  AVCodecContext* p_codec_ctx = NULL;
  AVRational frame_rate = { 0 };
  const AVRational* supported_framerates = NULL;
  AVDictionary* p_options = NULL;  // "create" an empty dictionary

  LOGD("%s %d E", __func__, __LINE__);

  if (m_p_video_stream) {
    res = ErrorDuplicatedInit;
    goto EXIT;
  }

  if (m_width <= 0 || m_height <= 0) {
    LOGE("Parameters is invalid to add new video stream : width=%d height=%d \n", m_width, m_height);
    res = ErrorParametersInvalid;
    goto EXIT;
  }

  /* find the video encoder */
  p_video_codec = avcodec_find_encoder(m_p_fmt_ctx->oformat->video_codec);
  //m_p_video_codec->type == AVMEDIA_TYPE_VIDEO; -- need to judge type in some case
  if (!p_video_codec) {
    res = ErrorNoEncoderFound;
    goto EXIT;
  }

  frame_rate = av_d2q(m_frame_rate, 1001000);
  supported_framerates = p_video_codec->supported_framerates;
  if (supported_framerates) {
    int idx = av_find_nearest_q_idx(frame_rate, supported_framerates);
    frame_rate = supported_framerates[idx];
  }

  m_p_video_stream = avformat_new_stream(m_p_fmt_ctx, p_video_codec);
  if (!m_p_video_stream) {
    LOGE("Cannot add new video stream\n");
    res = ErrorNoStreamFound;
    goto EXIT;
  }

  p_codec_ctx = m_p_video_stream->codec;
  p_codec_ctx->codec_id = m_p_fmt_ctx->oformat->video_codec;
  p_codec_ctx->codec_type = AVMEDIA_TYPE_VIDEO;
  //p_codec_ctx->frame_number = 0; //----

  // Put sample parameters.
  p_codec_ctx->bit_rate = m_video_bitrate;
  /* resolution must be a multiple of two, but ROUND up to 16 as often required */
  p_codec_ctx->width = m_width; //ALIGN2_16(m_width);
  p_codec_ctx->height = m_height;
  /* time base: this is the fundamental unit of time (in seconds) in terms
   of which frame timestamps are represented. for fixed-fps content,
   timebase should be 1/framerate and timestamp increments should be
   identically 1. */
  p_codec_ctx->time_base = av_inv_q(frame_rate);
  p_codec_ctx->gop_size = 12; // emit one intra frame every twelve frames at most
  //p_codec_ctx->max_b_frames = 1; //needed or "error like : encoder did not produce proper pts,making some up"

  if (p_codec_ctx->priv_data && p_codec_ctx->codec_id == AV_CODEC_ID_H264) {
    av_opt_set(p_codec_ctx->priv_data, "preset", "ultrafast", 0);
    //av_opt_set(p_codec_ctx->priv_data, "preset", "slow", 0);
    av_opt_set(p_codec_ctx->priv_data, "tune", "zerolatency", 0);
    //av_opt_set(p_codec_ctx->priv_data, "tune", "touhou", 0);
  }

  if (m_video_quality >= 0) {
    p_codec_ctx->flags |= CODEC_FLAG_QSCALE;
    p_codec_ctx->global_quality = (int)round(FF_QP2LAMBDA * m_video_quality);
  }

  av_opt_set_int(p_codec_ctx, "threads", 4, 0);
  //p_codec_ctx->thread_count = 4;

  if (m_pix_fmt != AV_PIX_FMT_NONE) {
    p_codec_ctx->pix_fmt = m_pix_fmt;
  } else if (p_codec_ctx->codec_id == AV_CODEC_ID_RAWVIDEO
    || p_codec_ctx->codec_id == AV_CODEC_ID_PNG
    || p_codec_ctx->codec_id == AV_CODEC_ID_HUFFYUV
    || p_codec_ctx->codec_id == AV_CODEC_ID_FFV1) {
    p_codec_ctx->pix_fmt = AV_PIX_FMT_RGB32; // appropriate for common lossless fmts
  } else {
    p_codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P; // lossy, but works with about everything
  }

  if (p_codec_ctx->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
    /* just for testing, we also add B frames */
    p_codec_ctx->max_b_frames = 2;
  } else if (p_codec_ctx->codec_id == AV_CODEC_ID_MPEG1VIDEO) {
    /* Needed to avoid using macroblocks in which some coeffs overflow.
     This does not happen with normal video, it just happens here as
     the motion of the chroma plane does not match the luma plane. */
    p_codec_ctx->mb_decision = 2;
  } else if (p_codec_ctx->codec_id == AV_CODEC_ID_H263) {
    // H.263 does not support any other resolution than the following
    if (m_width <= 128 && m_height <= 96) {
      p_codec_ctx->width = 128;
      p_codec_ctx->height = 96;
    } else if (m_width <= 176 && m_height <= 144) {
      p_codec_ctx->width = 176;
      p_codec_ctx->height = 144;
    } else if (m_width <= 352 && m_height <= 288) {
      p_codec_ctx->width = 352;
      p_codec_ctx->height = 288;
    } else if (m_width <= 704 && m_height <= 576) {
      p_codec_ctx->width = 704;
      p_codec_ctx->height = 576;
    } else {
      p_codec_ctx->width = 1408;
      p_codec_ctx->height = 1152;
    }
  } else if (p_codec_ctx->codec_id == AV_CODEC_ID_H264) {
    // default to constrained baseline to produce content that plays back on anything,
    // without any significant tradeoffs for most use cases
    p_codec_ctx->profile = FF_PROFILE_H264_CONSTRAINED_BASELINE;
  }

  // some fmts want stream headers to be separate
  if (m_p_fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER) {
    p_codec_ctx->flags |= CODEC_FLAG_GLOBAL_HEADER;
  }

  if (p_video_codec->capabilities & CODEC_CAP_EXPERIMENTAL) {
    p_codec_ctx->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
  }

  if (m_video_quality >= 0) {
    char _quality[8] = { 0 };
    sprintf(_quality, "%d", (int)m_video_quality);
    res = av_dict_set(&p_options, "crf", _quality, 0);
    if (res < 0)
      goto EXIT;

    res = av_dict_set(&p_options, "threads", "auto", 0);
    if (res < 0)
      goto EXIT;
  }

  //  av_dict_set(&options, "crf", "bar", 0); // add an entry
  //    char *k = av_strdup("key");       // if your strings are already allocated,
  //    char *v = av_strdup("value");     // you can avoid copying them like this

  //  //java
  //  if (videoQuality >= 0) {
  //    av_dict_set(options, "crf", "" + videoQuality, 0);
  //  }
  //  for (Entry<String, String> e : videoOptions.entrySet()) {
  //    av_dict_set(options, e.getKey(), e.getValue(), 0);
  //  }

  // open the codec.
  res = avcodec_open2(p_codec_ctx, p_video_codec, &p_options);
  CHK_RES(res);

  res = 0;
EXIT:
  if (res) {
    release();
  }

  if (p_options) {
    av_dict_free(&p_options);
  }

  LOGD("%s %d X res=%d", __func__, __LINE__, res);

  return res;
}

int ffmpeg_adapter_encoder::open_video_stream(uint8_t* extradata, long length) {
  int res = -1;
  int i = 0;

  LOGD("%s %d E ", __func__, __LINE__);

  if (!extradata || length <= 0) {
    res = ErrorParametersInvalid;
    goto EXIT;
  }

  if (m_p_video_stream) {
    res = ErrorDuplicatedInit;
    goto EXIT;
  }

  if (m_width <= 0 || m_height <= 0) {
    LOGE("Parameters is invalid to add new video stream : width=%d height=%d \n", m_width, m_height);
    res = ErrorParametersInvalid;
    goto EXIT;
  }

  m_p_video_stream = avformat_new_stream(m_p_fmt_ctx, NULL);
  if (!m_p_video_stream) {
    LOGE("Cannot add new video stream\n");
    res = ErrorNoStreamFound;
    goto EXIT;
  }

  m_p_video_stream->codec->extradata = (uint8_t*)av_mallocz(length);
  m_p_video_stream->codec->extradata_size = length;
  memcpy(m_p_video_stream->codec->extradata, extradata, length);

  m_p_video_stream->codec->codec_id = m_p_fmt_ctx->oformat->video_codec = AV_CODEC_ID_H264;
  m_p_video_stream->codec->codec_type = AVMEDIA_TYPE_VIDEO;

  // Put sample parameters.
  m_p_video_stream->codec->bit_rate = m_video_bitrate;
  /* resolution must be a multiple of two, but ROUND up to 16 as often required */
  m_p_video_stream->codec->width = m_width; //ALIGN2_16(m_width);
  m_p_video_stream->codec->height = m_height;
  /* time base: this is the fundamental unit of time (in seconds) in terms
  of which frame timestamps are represented. for fixed-fps content,
  timebase should be 1/framerate and timestamp increments should be
  identically 1. */
  m_p_video_stream->codec->time_base = av_inv_q(av_d2q(m_frame_rate, 1001000));

  m_p_video_stream->codec->coded_height = m_width;
  m_p_video_stream->codec->coded_width = m_height;
  m_p_video_stream->codec->pix_fmt = AV_PIX_FMT_YUV420P;
  m_p_video_stream->codec->max_b_frames = 0;
  m_p_video_stream->codec->profile = FF_COMPLIANCE_EXPERIMENTAL;

  res = 0;
EXIT:
  if (res) {

  }

  LOGD("%s %d X res=%d", __func__, __LINE__, res);

  return res;
}


int ffmpeg_adapter_encoder::open_audio_stream() {
  int res = -1;
  AVCodec* p_audio_codec = NULL;
  AVCodecContext *p_codec_ctx = NULL;
  AVDictionary* p_options = NULL; // "create" an empty dictionary

  LOGD("%s %d E ", __func__, __LINE__);

  if (m_p_audio_stream) {
    res = ErrorDuplicatedInit;
    goto EXIT;
  }

  if (m_audio_channels <= 0 || m_audio_bitrate <= 0 || m_samplerate <= 0) {
    res = ErrorParametersInvalid;
    goto EXIT;
  }

  /* find the audio encoder */
  p_audio_codec = avcodec_find_encoder(m_p_fmt_ctx->oformat->audio_codec);
  if (!p_audio_codec) {
    res = ErrorNoEncoderFound;
    goto EXIT;
  }

  // Try create stream.
  m_p_audio_stream = avformat_new_stream(m_p_fmt_ctx, p_audio_codec);
  if (!m_p_audio_stream) {
    LOGE("Cannot add new audio stream\n");
    res = ErrorNoStreamFound;
    goto EXIT;
  }

  // Codec.
  p_codec_ctx = m_p_audio_stream->codec;
  p_codec_ctx->codec_id = m_p_fmt_ctx->oformat->audio_codec;
  p_codec_ctx->codec_type = AVMEDIA_TYPE_AUDIO;

  /* put sample parameters */
  p_codec_ctx->bit_rate = m_audio_bitrate;
  p_codec_ctx->sample_rate = m_samplerate;
  p_codec_ctx->channels = m_audio_channels;
  p_codec_ctx->channel_layout = av_get_default_channel_layout(
    p_codec_ctx->channels);

  if (m_sample_fmt != AV_SAMPLE_FMT_NONE) {
    p_codec_ctx->sample_fmt = m_sample_fmt;
  } else if (p_codec_ctx->codec_id == AV_CODEC_ID_AAC
    && (p_audio_codec->capabilities & CODEC_CAP_EXPERIMENTAL)) {
    p_codec_ctx->sample_fmt = AV_SAMPLE_FMT_FLTP;
  } else {
    p_codec_ctx->sample_fmt = AV_SAMPLE_FMT_S16;
  }

  p_codec_ctx->time_base.num = 1;
  p_codec_ctx->time_base.den = m_samplerate;

  switch (p_codec_ctx->sample_fmt) {
  case AV_SAMPLE_FMT_U8:
  case AV_SAMPLE_FMT_U8P:
    p_codec_ctx->bits_per_raw_sample = 8;
    break;
  case AV_SAMPLE_FMT_S16:
  case AV_SAMPLE_FMT_S16P:
    p_codec_ctx->bits_per_raw_sample = 16;
    break;
  case AV_SAMPLE_FMT_S32:
  case AV_SAMPLE_FMT_S32P:
    p_codec_ctx->bits_per_raw_sample = 32;
    break;
  case AV_SAMPLE_FMT_FLT:
  case AV_SAMPLE_FMT_FLTP:
    p_codec_ctx->bits_per_raw_sample = 32;
    break;
  case AV_SAMPLE_FMT_DBL:
  case AV_SAMPLE_FMT_DBLP:
    p_codec_ctx->bits_per_raw_sample = 64;
    break;
  default:
    res = ErrorParametersInvalid;
    goto EXIT;
  }

  if (m_audio_quality >= 0) {
    p_codec_ctx->flags |= CODEC_FLAG_QSCALE;
    p_codec_ctx->global_quality = ROUND(FF_QP2LAMBDA * m_audio_quality);
  }

  // Some fmts want stream headers to be separate.
  if (m_p_fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER) {
    p_codec_ctx->flags |= CODEC_FLAG_GLOBAL_HEADER;
  }

  if (p_audio_codec->capabilities & CODEC_CAP_EXPERIMENTAL) {
    p_codec_ctx->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
  }

  if (m_audio_quality >= 0) {
    char _quality[8] = { 0 };
    sprintf(_quality, "%d", (int)m_audio_quality);
    res = av_dict_set(&p_options, "crf", _quality, 0);
    if (res < 0)
      goto EXIT;

    res = av_dict_set(&p_options, "threads", "auto", 0);
    if (res < 0)
      goto EXIT;
  }

  // open the codec.
  res = avcodec_open2(p_codec_ctx, p_audio_codec, &p_options);
  CHK_RES(res);

  res = 0;
EXIT:
  if (res) {
    release();
  }

  if (p_options) {
    av_dict_free(&p_options);
  }

  LOGD("%s %d X res=%d", __func__, __LINE__, res);

  return res;
}

int ffmpeg_adapter_encoder::open_audio_stream(uint8_t* extradata, long length) {
  int res = -1;
  int i = 0;

  LOGD("%s %d E ", __func__, __LINE__);

  if (!extradata || length <= 0) {
    res = ErrorParametersInvalid;
    goto EXIT;
  }

  if (m_p_audio_stream) {
    res = ErrorDuplicatedInit;
    goto EXIT;
  }

  m_p_audio_stream = avformat_new_stream(m_p_fmt_ctx, NULL);
  if (!m_p_audio_stream) {
    LOGE("Cannot add new video stream\n");
    res = ErrorNoStreamFound;
    goto EXIT;
  }

  m_p_audio_stream->codec->extradata = (uint8_t*)av_mallocz(length);
  m_p_audio_stream->codec->extradata_size = length;
  memcpy(m_p_audio_stream->codec->extradata, extradata, length);

  m_p_audio_stream->codec->codec_id = m_p_fmt_ctx->oformat->audio_codec = AV_CODEC_ID_AAC;
  m_p_audio_stream->codec->codec_type = AVMEDIA_TYPE_AUDIO;

  /* put sample parameters */
  m_p_audio_stream->codec->bit_rate = m_audio_bitrate;
  m_p_audio_stream->codec->sample_rate = m_samplerate;
  m_p_audio_stream->codec->channels = m_audio_channels;
  m_p_audio_stream->codec->channel_layout = av_get_default_channel_layout(
    m_p_audio_stream->codec->channels);

  m_p_audio_stream->codec->time_base.num = 1;
  m_p_audio_stream->codec->time_base.den = m_samplerate;

  m_p_audio_stream->codec->sample_fmt = AV_SAMPLE_FMT_S16;
  m_p_audio_stream->codec->bits_per_raw_sample = 16;

  m_p_audio_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;

  res = 0;
EXIT:
  if (res) {

  }

  LOGD("%s %d X res=%d", __func__, __LINE__, res);

  return res;
}

