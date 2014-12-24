// Main file

#include "stdafx.h"

#include "ffmpeg_adapter_common.h"
#include "ffmpeg_adapter_util.h"
#include "ffmpeg_adapter_encoder.h"
#include "ffmpeg_adapter_decoder.h"
#include "ffmpeg_adapter_jni.h"

extern "C" {
#include <avcodec.h>
#include <avformat.h>
#include <swscale.h>
#include <swresample.h>
#include <error.h>
#include "libavformat/avio.h"
#include "libavutil/audio_fifo.h"
#include "libavutil/avassert.h"
#include "libavutil/avstring.h"
#include "libavutil/frame.h"
#include "libavutil/opt.h"
}

//TODO ----------------------- memory leak

#define CAMERA_FILE_NAME "d:/test_files/camera.mp4"
//#define CAMERA_FILE_NAME "d:/test_files/48000.wav"
//#define OUTPUT_YUV_NAME "d:/test_files/result.mp4"
#define OUTPUT_YUV_NAME "d:/test_files/result.yuv"
#define INPUT_YUV_NAME "d:/test_files/yuv.yuv" //480x480
#define INPUT_YUV_NAME2 "d:/test_files/yuv2.yuv" //480x480
#define INPUT_PCM_NAME "d:/test_files/44100_16b.pcm" //44100 16bit 
#define INPUT_YUVSP_NAME "d:/test_files/640x480_420sp.yuv" //640x480

#define OUTPUT_NAME "d:/test_files/result.mp4"
#define INPUT_NAME "d:/test_files/8000.amr"
//#define INPUT_NAME "d:/test_files/test.ogg"
//#define INPUT_NAME "d:/test_files/test.flac"
//#define INPUT_NAME "d:/test_files/test.wma"
//#define INPUT_NAME "d:/test_files/test.ape" //-- not support
//#define INPUT_NAME "d:/test_files/test.wav"

#define CONCAT_INPUT_NAME0 "d:/temp/test.mp4"
#define CONCAT_INPUT_NAME1 "d:/temp/nodata.mp4"
#define CONCAT_OUTPUT_NAME "d:/temp/result.mp4"

#define JNI_TEST (0x1)
#define JNI_TEST_ENCODE_BUFFER (0x2)
#define TEST_AUDIO (0x4)
#define TEST_VIDEO (0x8)
#define TEST_DECODE_VIDEO (0x10)
#define TEST_CONCAT (0x20)
#define TEST_CROP (0x40)

#define _FLAG (TEST_AUDIO|TEST_VIDEO)

double fps = 15;
int w = 480;
int h = 480;
long frame_count = 225;
int quality = 20;
long seekFrom = 240000000;
long duration = 15000000;
int single_frame_show_times = frame_count / 6; //missing some frames

static void jni_test() {
  int res = 0;

  int context = _nativeInit();
  int context_audio = _nativeInit();
  if (!context) {
    res = -1;
    goto EXIT;
  }

  if (1) {
    w = 720;
    h = 480;

    res = _nativeEncodeSetFps(context, fps);
    if (res) {
      goto EXIT;
    }

    res = _nativeEncodeSetVideoQuality(context, quality);
    if (res) {
      goto EXIT;
    }

    res = _nativeEncodeSetResolution(context, h, h);
    if (res) {
      goto EXIT;
    }

    res = _nativeDecodeFrom(context, CAMERA_FILE_NAME);
    res = _nativeDecodeFrom(context_audio, INPUT_NAME);
    if (res) {
      goto EXIT;
    }

    jbyteArray rgb_array = { 0 };
    rgb_array.length = h * h * 4;
    rgb_array.data = (uint8_t*)calloc(1, sizeof(uint8_t)* rgb_array.length);

    jbyteArray yuv_array = { 0 };
    yuv_array.length = h * h * 3 / 2;
    yuv_array.data = (uint8_t*)calloc(1, sizeof(uint8_t)* yuv_array.length);

    jbyteArray pcm_array = { 0 };
    pcm_array.length = _nativeDecodeGetAudioBufferSize(context_audio);
    pcm_array.data = (uint8_t*)calloc(1, sizeof(uint16_t)* pcm_array.length);

    int last_pcm_length = 0;

    int x = _nativeDecodeGetAudioBufferSize(context);

    if (_FLAG & JNI_TEST_ENCODE_BUFFER) {
      FILE * pTempFile = fopen(INPUT_YUV_NAME, "rb");
      int x = fread(yuv_array.data, sizeof(uint8_t), yuv_array.length / sizeof(uint8_t), pTempFile);
      fclose(pTempFile);

      FILE * p_pcm_file = fopen(INPUT_PCM_NAME, "rb");

      //--test encode buffer
      res = _nativeEncodeTo(context, OUTPUT_NAME);
      for (int i = 0; i < 100; i++){
        res = _nativeEncodeBufferV(context, i, yuv_array, 0, 480, 480, 0);
        if (res) {
          goto EXIT;
        }

        while (_nativeDecodeActualTimestampA(context_audio) < _nativeEncodeGetDurationV(context)) {
          res = _nativeDecodeFrameA(context_audio, 0, pcm_array, 0, 100);
          if (res) {
            goto EXIT;
          }

          int iABufferSize = _nativeDecodeGetAudioBufferSize(context_audio);

          res = _nativeEncodeBufferA(context, _nativeEncodeGetDurationA(context), pcm_array, 0, iABufferSize);
          if (res) {
            goto EXIT;
          }
        }
        
        /*if (fread(pcm_array.data, sizeof(uint8_t), pcm_array.length / sizeof(uint8_t), p_pcm_file)) {
          res = _nativeEncodeAudioBuffer(context, 0, pcm_array, 0, pcm_array.length);
          if (res) {
          goto EXIT;
          }
          }*/
      }

      fclose(p_pcm_file);

      goto EXIT;
    }

    jbyteArray arrayAudio = { 0 };
    arrayAudio.length = _nativeDecodeGetAudioBufferSize(context);
    arrayAudio.data = (uint8_t*)calloc(1, sizeof(uint8_t)* arrayAudio.length);

    long timestamp = 100000;
    long audio_timestamp = 0;
    long timeintercal = 30000;
    int frame_count = 0;
    while (!_nativeDecodeEoF(context)) {
      res = _nativeDecodeFrameV(context, timestamp, rgb_array, 0, 480, 480, 0);
      if (_nativeDecodeEoF(context)) {
        break;
      }
      if (res) {
        goto EXIT;
      }

      //res = _nativeRgbaToYuv(context, rgb_array, yuv_array, 480, 480, 0x15);
      //dump_to_file("d:/xxx.yuv", yuv_array.data, yuv_array.length);

      frame_count++;

      timestamp = _nativeDecodeActualTimestampV(context);
    }

    _nativeDecodeSeekTo(context, 0);
    while (true) {
      res = _nativeDecodeFrameV(context, timestamp, yuv_array, 1, 480, 480, 0);
      if (_nativeDecodeEoF(context)) {
        break;
      }
      if (res) {
        goto EXIT;
      }

      timestamp += timeintercal;

      if (_nativeDecodeEoF(context)) {
        break;
      }
    }
  } else {
    res = _nativeEncodeSetResolution(context, w, h);
    if (res) {
      goto EXIT;
    }

    res = _nativeSetAudioStartFrom(context, 0);
    if (res) {
      goto EXIT;
    }

    res = _nativeEncodeSetFps(context, fps);
    if (res) {
      goto EXIT;
    }

    res = _nativeEncodeSetVideoQuality(context, quality);
    if (res) {
      goto EXIT;
    }

    res = _nativeSetEncodeDuration(context, duration);
    if (res) {
      goto EXIT;
    }

    res = _nativeEncodeTo(context, OUTPUT_NAME);
    if (res) {
      goto EXIT;
    }

    res = _nativeDecodeFrom(context, INPUT_NAME);
    if (res) {
      goto EXIT;
    }
  }



EXIT:

  if (res) {

  }

  _nativeRelease(context);
}

static void common_test() {
  int res = 0;


  ffmpeg_adapter_encoder* encoder = new ffmpeg_adapter_encoder();
  ffmpeg_adapter_decoder* decoder = new ffmpeg_adapter_decoder();
  AVFrame* frame = NULL;

  /*if (!HAS_AUDIO && !HAS_VIDEO && !_DECODE_VIDEO) {
  LOGE("YOU MUST BE KIDDING! -_-");
  goto EXIT;
  }*/

  if (!encoder || !decoder) {
    LOGE("WHAT?! -_-");
    goto EXIT;
  }

  encoder->set_resolution(w, h);
  encoder->set_fps(fps);
  encoder->set_video_quality(quality);
  encoder->enable_fake_audio(true);
  encoder->set_output_path(OUTPUT_NAME);

  if (_FLAG & TEST_AUDIO || _FLAG & TEST_DECODE_VIDEO) {
    CHK_RES(decoder->start(INPUT_NAME));
    if (decoder->seek_to(seekFrom)) {
      decoder->seek_to(0);
      seekFrom = 0;
    }

    //CHK_RES(encoder->set_audio_channels(decoder->get_audio_channels()));
    //CHK_RES(encoder->set_samplerate(decoder->get_audio_samplerate()));
  }


  //test extra data
  /*uint8_t* xxx = new uint8_t[21];
  FILE * pTempFile = fopen("d:/configure.data", "rb");
  int x = fread(xxx, sizeof(uint8_t), 21 / sizeof(uint8_t), pTempFile);
  fclose(pTempFile);*/

  char filename[128] = { 0 };
  if (_FLAG & (TEST_VIDEO) && !(_FLAG & TEST_DECODE_VIDEO)) {
    int i, j;
    for (i = 0; i < 6; i++) {
      sprintf(filename, "d:/temp/%d.jpg", i);
      for (j = 0; j < single_frame_show_times; j++) {
        encoder->add_image_file(filename, w, h, j + i * single_frame_show_times);

        LOGE("timestamp=%ld \n", encoder->get_current_video_duration());
      }
    }

    if (_FLAG & TEST_AUDIO) {
      long audio_current_timestamp = 0;
      long rest_timestamp = 0;
      while ((audio_current_timestamp = decoder->get_audio_timestamp() - seekFrom) <= encoder->get_current_video_duration())  {
        frame = decoder->get_audio_frame();
        if (frame) {
          //LOGE("duration = %ld, current duration=%ld", duration, audio_current_timestamp);

          rest_timestamp = duration - audio_current_timestamp;
          rest_timestamp = rest_timestamp > 0 ? rest_timestamp : 0;
          if ((duration > 6000000L) && (rest_timestamp < 3000000L)) {
            scale_audio_frame_volume(frame, (double)(rest_timestamp) / 3000000L);
          }

          encoder->add_audio_frame(frame);
          RELEASE_FRAME(frame);
        } else {
          if (decoder->is_eof()) {
            //TODO -- to check the result
            encoder->add_audio_frame(NULL);
            break;
          } else {
            break;
          }
        }
      }

      if (encoder->use_fake_audio()) {
        while (encoder->get_current_audio_duration() <= encoder->get_current_video_duration()) {
          encoder->add_audio_frame(NULL);
        }
      }
    }



  } else if (_FLAG & TEST_AUDIO) {
    while (frame = decoder->get_audio_frame())  {
      encoder->add_audio_frame(frame);
      RELEASE_FRAME(frame);
    }

    encoder->add_audio_frame(NULL);
  } else if (_FLAG & TEST_DECODE_VIDEO) {
    while (frame = decoder->get_video_frame())  {
      encoder->add_video_frame(frame);
      RELEASE_FRAME(frame);
    }

    encoder->add_video_frame(NULL);
  }

  if (encoder) {
    encoder->add_audio_frame(NULL);
    encoder->add_video_frame(NULL);

    delete encoder;
    encoder = NULL;
  }

  if (decoder) {
    delete decoder;
    decoder = NULL;
  }


EXIT:
  //return 0;

  return;
}

static void concat_test() {
  char** xxx = new char*[1];
  xxx[0] = CONCAT_INPUT_NAME0;
  xxx[1] = CONCAT_INPUT_NAME1;
  /*xxx[2] = CONCAT_INPUT_NAME0;
  xxx[3] = CONCAT_INPUT_NAME1;
  xxx[4] = CONCAT_INPUT_NAME0;*/

  int res = concat_files((const char **)xxx, 2, CONCAT_OUTPUT_NAME);
}

static void crop_test() {
  int w = 640;
  int h = 480;

  uint8_t* p_result_buffer = (uint8_t*)calloc(1, sizeof(uint8_t)* h * h * 3 / 2);

  jbyteArray array = { 0 };
  array.length = w * h * 3 / 2;
  array.data = (uint8_t*)calloc(1, sizeof(uint8_t)* array.length);

  FILE * pTempFile = fopen(INPUT_YUVSP_NAME, "rb");
  int x = fread(array.data, sizeof(uint8_t), array.length / sizeof(uint8_t), pTempFile);
  fclose(pTempFile);

  int res = YUV420SP_crop(array.data, p_result_buffer, w, h, 80, 0, h, h);

  dump_to_file(OUTPUT_YUV_NAME, p_result_buffer, h * h * 3 / 2);

  free(array.data);
  free(p_result_buffer);
}

int _tmain(int argc, _TCHAR* argv[]) {
  if (_FLAG & (JNI_TEST | JNI_TEST_ENCODE_BUFFER)) {
    jni_test();
  } else if (_FLAG & (TEST_AUDIO | TEST_DECODE_VIDEO | TEST_VIDEO)) {
    common_test();
  } else if (_FLAG & TEST_CONCAT) {
    concat_test();
  } else if (_FLAG & TEST_CROP) {
    crop_test();
  }

  _CrtDumpMemoryLeaks();

EXIT:
  return 0;
}

