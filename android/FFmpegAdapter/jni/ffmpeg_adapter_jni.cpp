/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#ifdef __ANDROID__
#include <string.h>
#include <jni.h>
#include <android/bitmap.h>
#endif
#include "ffmpeg_adapter_common.h"
#include "ffmpeg_adapter_jni.h"
#include "ffmpeg_adapter_encoder.h"
#include "ffmpeg_adapter_decoder.h"
#include "ffmpeg_adapter_converter.h"
#include "ffmpeg_adapter_transform.h"
//#include <GraphicsJNI.h>

//#define ENABLE_DUMP 1

#ifndef ENABLE_DUMP
#define dump_to_file 
#endif //ENABLE_DUMP

#ifdef __ANDROID__
#define DUMP_PATH(x) generate_path_name("/sdcard/test_files/", x)
#else //__ANDROID__
//#define DUMP_PATH(x) generate_path_name("d:/test_files/", x)
#define DUMP_PATH(x) "d:/temp/"##x
#endif //__ANDROID__

typedef struct _ffmpeg_adapter_argb_t {
  uint8_t alpha;
  uint8_t red;
  uint8_t green;
  uint8_t blue;
} argb;

#define TAG ("ffmpeg_adapter_jni")

typedef struct _ffmpeg_adapter_context_struct_ {
  ffmpeg_adapter_encoder* p_encoder;
  ffmpeg_adapter_decoder* p_decoder;
  long from_timestamp;
  int width;
  int height;
  long duration;

  int nb_samples;
  int format;
  int channels;
  int channel_layout;
  int sample_rate;

  int last_error;

  long decode_buffer_length;
  long encode_buffer_length;
  uint8_t* temp_decode_crop_buffer;
  uint8_t* temp_decode_rotate_buffer;
  uint8_t* temp_encode_buffer;
} ffmpeg_adapter_context, *p_ffmpeg_adapter_context;

#ifdef __cplusplus
extern "C" {
#endif 

  static void native_ffmpeg_adapter_end(ffmpeg_adapter_context* p_context);

  static int convert_rgba_to_yuv(uint8_t* rgba, uint8_t* yuv, int width, int height, int flag);

  static int fill_audio(ffmpeg_adapter_context* p_context);

#ifdef __cplusplus
}
#endif 

static const char* getString(JNIEnv* env, jstring string) {
#ifdef __ANDROID__
  if(!string)
    return NULL;

  return env->GetStringUTFChars(string, 0);
#else
  return (const char*)string; //should be char*
#endif
}

static void releaseString(JNIEnv* env, jstring string, const char* array) {
#ifdef __ANDROID__
  env->ReleaseStringUTFChars(string, array);
#else
  return;
#endif
}

static jbyte* getByteArray(JNIEnv* env, jbyteArray data) {
#ifdef __ANDROID__
  if(!data)
    return NULL;

  return env->GetByteArrayElements(data, 0);
#else
  return data.data; //should be char*
#endif
}

static jsize getByteArrayLength(JNIEnv* env, jbyteArray data) {
#ifdef __ANDROID__
  return env->GetArrayLength(data);
#else
  return data.length; //should be char*
#endif
}

static void releaseByteArray(JNIEnv* env, jbyteArray data, jbyte* array) {
#ifdef __ANDROID__
  env->ReleaseByteArrayElements(data, array, 0);
#else
  return;
#endif
}

jint _JNI(nativeInit)(JNIEnv* env, jobject thiz) {
  int res = -1;
  ffmpeg_adapter_context* p_context = NULL;

#if defined(__arm__)
#if defined(__ARM_ARCH_7A__)
#if defined(__ARM_NEON__)
#define ABI "armeabi-v7a/NEON"
#else
#define ABI "armeabi-v7a"
#endif
#else
#define ABI "armeabi"
#endif
#elif defined(__i386__)
#define ABI "x86"
#elif defined(__mips__)
#define ABI "mips"
#else
#define ABI "unknown"
#endif

  LOGD("%s %d E   compiled with ABI %s \n", __func__, __LINE__, ABI);

  p_context = (ffmpeg_adapter_context*)calloc(1, sizeof(ffmpeg_adapter_context));
  if (!p_context) {
    res = ErrorAllocFailed;
    goto EXIT;
  }

  p_context->p_decoder = new ffmpeg_adapter_decoder();
  if (!p_context->p_decoder) {
    res = ErrorAllocFailed;
    goto EXIT;
  }

  p_context->p_encoder = new ffmpeg_adapter_encoder();
  if (!p_context->p_encoder) {
    res = ErrorAllocFailed;
    goto EXIT;
  }

  TransformTableInit();

  res = 0;
EXIT:
  if (p_context) {
    p_context->last_error = res;
  }

  //DO NOT release here, let .java do this.
  //  if (res) {
  //    native_ffmpeg_adapter_end(p_context);
  //    p_context = NULL;
  //  }

  LOGD("%s %d X context=%p res=%d \n", __func__, __LINE__, p_context, res);

  return (int)p_context;
}

void _JNI(nativeRelease)(JNIEnv* env, jobject thiz,
  jint context) {
  int res = -1;
  ffmpeg_adapter_context* p_context = (ffmpeg_adapter_context*)context;

  LOGD("%s %d E context=%d \n", __func__, __LINE__, context);

  if (p_context) {
    native_ffmpeg_adapter_end(p_context);
    delete p_context;
    p_context = NULL;
  }

  LOGD("%s %d X context=%d \n", __func__, __LINE__, context);

  return;
}

jint _JNI(nativeSetAudioStartFrom)(JNIEnv* env, jobject thiz,
  jint context,
  jlong from) {
  int res = -1;
  ffmpeg_adapter_context* p_context = (ffmpeg_adapter_context*)context;

  LOGD("%s %d E context=%d from=%ld \n", __func__, __LINE__, context, (long)from);
  if (!p_context) {
    res = ErrorNoContext;
    goto EXIT;
  }

  //to add more judgments
  if (from < 0){
    res = ErrorParametersInvalid;
    goto EXIT;
  }

  p_context->from_timestamp = from;

  res = 0;
EXIT:
  if (res) {

  }

  if (p_context) {
    p_context->last_error = res;
  }

  LOGD("%s %d X res=%d \n", __func__, __LINE__, res);

  return res;
}

jint _JNI(nativeSetEncodeDuration)(JNIEnv* env, jobject thiz,
  jint context,
  jlong duration) {
  int res = -1;
  ffmpeg_adapter_context* p_context = (ffmpeg_adapter_context*)context;

  LOGD("%s %d E context=%d duration=%ld \n", __func__, __LINE__, context, (long)duration);
  if (!p_context) {
    res = ErrorNoContext;
    goto EXIT;
  }

  if (duration < 200000){
    res = ErrorParametersInvalid;
    goto EXIT;
  }

  p_context->duration = duration;

  res = 0;
EXIT:
  if (res) {

  }

  if (p_context) {
    p_context->last_error = res;
  }

  LOGD("%s %d X res=%d \n", __func__, __LINE__, res);

  return res;
}


jint _JNI(nativeDecodeFrom)(JNIEnv* env, jobject thiz,
  jint context, jstring filepath) {
  int res = -1;
  const char *path = NULL;
  ffmpeg_adapter_context* p_context = (ffmpeg_adapter_context*)context;

  LOGD("%s %d E path=%p \n", __func__, __LINE__, filepath);

  if (!p_context) {
    res = ErrorNoGlobalInstance;
    goto EXIT;
  }

  if (!filepath) {
    res = ErrorParametersInvalid;
    goto EXIT;
  }

  path = getString(env, filepath);

  res = p_context->p_decoder->start(path);
  if (res)
    goto EXIT;

  //try to seek
  if (p_context->from_timestamp) {
    res = p_context->p_decoder->seek_to(p_context->from_timestamp);
    //help fix one crash issue when the set to a invalid position
    //      if(res)
    //        goto EXIT;
  }

  res = p_context->p_encoder->enable_fake_audio(true);
  if (res)
    goto EXIT;


  //  //jclass clazz =env->FindClass("android/hardware/Camera$CameraInfo");
  //  clazz = env->GetObjectClass(thiz);
  //  if(!clazz) {
  //    LOGE("%s %d JNI get object class failed!", __func__, __LINE__);
  //    res = -1;
  //    goto EXIT;
  //  }
  //
  //  fid = env->GetFieldID(clazz,"mContext","I");
  //  if(!fid) {
  //    LOGE("%s %d JNI get filed id failed!", __func__, __LINE__);
  //    res = -1;
  //    goto EXIT;
  //  }
  //
  //  //need to check the fid value first, to avoid overwriting context.
  //  env->SetIntField(thiz ,fid, (int)p_context);

  res = 0;
EXIT:
  if (path) {
    releaseString(env, filepath, path);
  }

  if (p_context)
    p_context->last_error = res;

  LOGD("%s %d X context=%p res=%d \n", __func__, __LINE__, p_context, res);

  return res;
}

jint _JNI(nativeEncodeTo)(JNIEnv* env, jobject thiz, jint context, jstring fileName) {
  int res = -1;
  const char *filename = NULL;
  ffmpeg_adapter_context* p_context = (ffmpeg_adapter_context*)context;

  LOGD("%s %d E fileName=%p \n", __func__, __LINE__, fileName);

  if (!p_context || !p_context->p_encoder) {
    res = ErrorNoGlobalInstance;
    goto EXIT;
  }

  if (!fileName) {
    res = ErrorParametersInvalid;
    goto EXIT;
  }

  filename = getString(env, fileName);
  LOGE("%s %d filename=%s \n", __func__, __LINE__, filename);
  if (!filename) {
    res = ErrorAllocFailed;
    goto EXIT;
  }

  res = p_context->p_encoder->set_output_path(filename);
  if (res)
    goto EXIT;

  res = 0;
EXIT:
  if (filename) {
    releaseString(env, fileName, filename);
  }

  if (p_context)
    p_context->last_error = res;

  LOGD("%s %d X context=%p res=%d \n", __func__, __LINE__, p_context, res);

  return res;
}

jlong _JNI(nativeEncodeGetDurationV)(JNIEnv* env, jobject thiz, jint context) {
  int res = -1;
  ffmpeg_adapter_context* p_context = (ffmpeg_adapter_context*)context;
  long timestamp = 0;

  LOGD("%s %d E context=%d \n", __func__, __LINE__, context);

  if (!p_context || !p_context->p_encoder) {
    res = ErrorNoContext;
    goto EXIT;
  }

  timestamp = p_context->p_encoder->get_current_video_duration();

  res = 0;
EXIT:
  if (res) {
    timestamp = -1;
  }

  LOGD("%s %d X context=%d timestamp=%ld \n", __func__, __LINE__, context, timestamp);

  return static_cast<long>(timestamp);
}

jlong _JNI(nativeEncodeGetDurationA)(JNIEnv* env, jobject thiz, jint context) {
  int res = -1;
  ffmpeg_adapter_context* p_context = (ffmpeg_adapter_context*)context;
  long timestamp = 0;

  LOGD("%s %d E context=%d \n", __func__, __LINE__, context);

  if (!p_context || !p_context->p_encoder) {
    res = ErrorNoContext;
    goto EXIT;
  }

  timestamp = p_context->p_encoder->get_current_audio_duration();

  res = 0;
EXIT:
  if (res) {
    timestamp = -1;
  }

  LOGD("%s %d X context=%d timestamp=%ld \n", __func__, __LINE__, context, timestamp);

  return static_cast<long>(timestamp);
}

jint _JNI(nativeEncodeSetResolution)(JNIEnv* env, jobject thiz,
  jint context,
  jint width, jint height) {
  int res = -1;
  ffmpeg_adapter_context* p_context = (ffmpeg_adapter_context*)context;

  LOGD("%s %d E context=%d width=%d height=%d \n", __func__, __LINE__, context, width, height);
  if (!p_context) {
    res = ErrorNoContext;
    goto EXIT;
  }

  if (width <= 0 || height <= 0 || width % 16) {
    res = ErrorParametersInvalid;
    goto EXIT;
  }

  p_context->width = width;
  p_context->height = height;

  if (p_context->p_encoder) {
    res = p_context->p_encoder->set_resolution(width, height);
    if (res)
      goto EXIT;
  }

  res = 0;
EXIT:
  if (res) {

  }

  if (p_context) {
    p_context->last_error = res;
  }

  LOGD("%s %d X res=%d \n", __func__, __LINE__, res);

  return res;
}

jint _JNI(nativeEncodeSetFps)(JNIEnv* env, jobject thiz,
  jint context,
  jdouble fps) {
  int res = -1;
  ffmpeg_adapter_context* p_context = (ffmpeg_adapter_context*)context;

  LOGD("%s %d E context=%d fps=%f \n", __func__, __LINE__, context, fps);
  if (!p_context || !p_context->p_encoder) {
    res = ErrorNoContext;
    goto EXIT;
  }

  if (fps < 2 || fps > 60){
    res = ErrorParametersInvalid;
    goto EXIT;
  }

  res = p_context->p_encoder->set_fps(fps);
  if (res)
    goto EXIT;

  res = 0;
EXIT:
  if (res) {

  }

  if (p_context) {
    p_context->last_error = res;
  }

  LOGD("%s %d X res=%d \n", __func__, __LINE__, res);

  return res;
}

jint _JNI(nativeEncodeSetVideoQuality)(JNIEnv* env, jobject thiz,
  jint context,
  jint video_quality) {
  int res = -1;
  ffmpeg_adapter_context* p_context = (ffmpeg_adapter_context*)context;

  LOGD("%s %d E context=%d video_quality=%d \n", __func__, __LINE__, context, video_quality);
  if (!p_context || !p_context->p_encoder) {
    res = ErrorNoContext;
    goto EXIT;
  }

  if (video_quality < 0 || video_quality > 100){
    res = ErrorParametersInvalid;
    goto EXIT;
  }

  res = p_context->p_encoder->set_video_quality(video_quality);
  if (res)
    goto EXIT;

  res = 0;
EXIT:
  if (res) {

  }

  if (p_context) {
    p_context->last_error = res;
  }

  LOGD("%s %d X res=%d \n", __func__, __LINE__, res);

  return res;
}

jlong _JNI(nativeDecodeGetDurationV)(JNIEnv* env, jobject thiz, jint context) {
  int res = -1;
  ffmpeg_adapter_context* p_context = (ffmpeg_adapter_context*)context;
  long result = 0;

  LOGD("%s %d E context=%d \n", __func__, __LINE__, context);

  if (!p_context || !p_context->p_decoder) {
    res = ErrorNoContext;
    goto EXIT;
  }

  result = p_context->p_decoder->get_video_duration();

  res = 0;
EXIT:
  if (res) {
    result = 0;
  }

  LOGD("%s %d X context=%d \n", __func__, __LINE__, context);

  return result;
}

jlong _JNI(nativeDecodeGetDurationA)(JNIEnv* env, jobject thiz, jint context) {
  int res = -1;
  ffmpeg_adapter_context* p_context = (ffmpeg_adapter_context*)context;
  long result = 0;

  LOGD("%s %d E context=%d \n", __func__, __LINE__, context);

  if (!p_context || !p_context->p_decoder) {
    res = ErrorNoContext;
    goto EXIT;
  }

  result = p_context->p_decoder->get_audio_duration();

  res = 0;
EXIT:
  if (res) {
    result = 0;
  }

  LOGD("%s %d X context=%d \n", __func__, __LINE__, context);

  return result;
}

jint _JNI(nativeDecodeGetAudioFormat)(JNIEnv* env, jobject thiz, jint context) {
  int res = -1;
  ffmpeg_adapter_context* p_context = (ffmpeg_adapter_context*)context;
  int result = 0;

  LOGD("%s %d E context=%d \n", __func__, __LINE__, context);

  if (!p_context || !p_context->p_decoder) {
    res = ErrorNoContext;
    goto EXIT;
  }

  result = p_context->p_decoder->get_audio_format();

  res = 0;
EXIT:
  if (res) {
    result = 0;
  }

  LOGD("%s %d X context=%d \n", __func__, __LINE__, context);

  return result;
}

jint _JNI(nativeDecodeGetAudioSamplerate)(JNIEnv* env, jobject thiz, jint context) {
  int res = -1;
  ffmpeg_adapter_context* p_context = (ffmpeg_adapter_context*)context;
  int result = 0;

  LOGD("%s %d E context=%d \n", __func__, __LINE__, context);

  if (!p_context || !p_context->p_decoder) {
    res = ErrorNoContext;
    goto EXIT;
  }

  result = p_context->p_decoder->get_audio_samplerate();

  res = 0;
EXIT:
  if (res) {
    result = 0;
  }

  LOGD("%s %d X context=%d \n", __func__, __LINE__, context);

  return result;
}

jint _JNI(nativeDecodeGetAudioChannels)(JNIEnv* env, jobject thiz, jint context) {
  int res = -1;
  ffmpeg_adapter_context* p_context = (ffmpeg_adapter_context*)context;
  int result = 0;

  LOGD("%s %d E context=%d \n", __func__, __LINE__, context);

  if (!p_context || !p_context->p_decoder) {
    res = ErrorNoContext;
    goto EXIT;
  }

  result = p_context->p_decoder->get_audio_channels();

  res = 0;
EXIT:
  if (res) {
    result = 0;
  }

  LOGD("%s %d X context=%d \n", __func__, __LINE__, context);

  return result;
}

jint _JNI(nativeDecodeGetAudioChannelLayout)(JNIEnv* env, jobject thiz, jint context) {
  int res = -1;
  ffmpeg_adapter_context* p_context = (ffmpeg_adapter_context*)context;
  int result = 0;

  LOGD("%s %d E context=%d \n", __func__, __LINE__, context);

  if (!p_context || !p_context->p_decoder) {
    res = ErrorNoContext;
    goto EXIT;
  }

  result = p_context->p_decoder->get_audio_channel_layout();

  res = 0;
EXIT:
  if (res) {
    result = 0;
  }

  LOGD("%s %d X context=%d \n", __func__, __LINE__, context);

  return result;
}

jint _JNI(nativeDecodeGetAudioFrameSize)(JNIEnv* env, jobject thiz, jint context) {
  int res = -1;
  ffmpeg_adapter_context* p_context = (ffmpeg_adapter_context*)context;
  int result = 0;

  LOGD("%s %d E context=%d \n", __func__, __LINE__, context);

  if (!p_context || !p_context->p_decoder) {
    res = ErrorNoContext;
    goto EXIT;
  }

  result = p_context->p_decoder->get_audio_framesize();

  res = 0;
EXIT:
  if (res) {
    result = 0;
  }

  LOGD("%s %d X context=%d \n", __func__, __LINE__, context);

  return result;
}

jint _JNI(nativeDecodeGetAudioBufferSize)(JNIEnv* env, jobject thiz, jint context) {
  int res = -1;
  ffmpeg_adapter_context* p_context = (ffmpeg_adapter_context*)context;
  int result = 0;

  LOGD("%s %d E context=%d \n", __func__, __LINE__, context);

  if (!p_context || !p_context->p_decoder) {
    res = ErrorNoContext;
    goto EXIT;
  }

  result = p_context->p_decoder->get_output_audio_buffer_size();

  res = 0;
EXIT:
  if (res) {
    LOGE("%s %d Error res=%d \n", __func__, __LINE__, res);
    result = 0;
  }

  LOGD("%s %d X context=%d \n", __func__, __LINE__, context);

  return result;
}

jint _JNI(nativeDecodeSetAudioFormat)(JNIEnv* env, jobject thiz, jint context, jint format) {
  int res = -1;
  ffmpeg_adapter_context* p_context = (ffmpeg_adapter_context*)context;

  LOGD("%s %d E context=%d format=%d \n", __func__, __LINE__, context, format);
  if (!p_context) {
    res = ErrorNoContext;
    goto EXIT;
  }

  if (format <= 0) {
    res = ErrorParametersInvalid;
    goto EXIT;
  }

  res = p_context->p_decoder->set_output_audio_format(format);
  if (res)
    goto EXIT;

  res = 0;
EXIT:
  if (res) {

  }

  if (p_context) {
    p_context->last_error = res;
  }

  LOGD("%s %d X res=%d \n", __func__, __LINE__, res);

  return res;
}

jint _JNI(nativeDecodeSetAudioSamplerate)(JNIEnv* env, jobject thiz, jint context, jint samplerate) {
  int res = -1;
  ffmpeg_adapter_context* p_context = (ffmpeg_adapter_context*)context;

  LOGD("%s %d E context=%d samplerate=%d \n", __func__, __LINE__, context, samplerate);
  if (!p_context) {
    res = ErrorNoContext;
    goto EXIT;
  }

  if (samplerate <= 0) {
    res = ErrorParametersInvalid;
    goto EXIT;
  }

  res = p_context->p_decoder->set_output_samplerate(samplerate);
  if (res)
    goto EXIT;

  res = 0;
EXIT:
  if (res) {

  }

  if (p_context) {
    p_context->last_error = res;
  }

  LOGD("%s %d X res=%d \n", __func__, __LINE__, res);

  return res;
}

jint _JNI(nativeDecodeSetAudioChannels)(JNIEnv* env, jobject thiz, jint context, jint channels) {
  int res = -1;
  ffmpeg_adapter_context* p_context = (ffmpeg_adapter_context*)context;

  LOGD("%s %d E context=%d channels=%d \n", __func__, __LINE__, context, channels);
  if (!p_context) {
    res = ErrorNoContext;
    goto EXIT;
  }

  if (channels <= 0) {
    res = ErrorParametersInvalid;
    goto EXIT;
  }

  res = p_context->p_decoder->set_output_audio_channels(channels);
  if (res)
    goto EXIT;

  res = 0;
EXIT:
  if (res) {

  }

  if (p_context) {
    p_context->last_error = res;
  }

  LOGD("%s %d X res=%d \n", __func__, __LINE__, res);

  return res;
}

jint _JNI(nativeDecodeSetAudioFrameSize)(JNIEnv* env, jobject thiz, jint context, jint framesize) {
  int res = -1;
  ffmpeg_adapter_context* p_context = (ffmpeg_adapter_context*)context;

  LOGD("%s %d E context=%d framesize=%d \n", __func__, __LINE__, context, framesize);
  if (!p_context) {
    res = ErrorNoContext;
    goto EXIT;
  }

  if (framesize <= 0) {
    res = ErrorParametersInvalid;
    goto EXIT;
  }

  res = p_context->p_decoder->set_output_audio_frame_size(framesize);
  if (res)
    goto EXIT;

  res = 0;
EXIT:
  if (res) {

  }

  if (p_context) {
    p_context->last_error = res;
  }

  LOGD("%s %d X res=%d \n", __func__, __LINE__, res);

  return res;
}

jint _JNI(nativeDecodeSeekTo)(JNIEnv* env, jobject thiz, jint context, jlong timestamp) {
  int res = -1;
  ffmpeg_adapter_context* p_context = (ffmpeg_adapter_context*)context;

  LOGD("%s %d E context=%d \n", __func__, __LINE__, context);

  if (!p_context || !p_context->p_decoder) {
    res = ErrorNoContext;
    goto EXIT;
  }

  res = p_context->p_decoder->seek_to(static_cast<long>(timestamp));
  if (res)
    goto EXIT;

  res = 0;
EXIT:
  if (res) {

  }

  LOGD("%s %d X context=%d \n", __func__, __LINE__, context);

  return res;
}

jint _JNI(nativeDecodeEoF)(JNIEnv* env, jobject thiz, jint context) {
  int res = -1;
  ffmpeg_adapter_context* p_context = (ffmpeg_adapter_context*)context;
  int eof = 1;

  LOGD("%s %d E context=%d \n", __func__, __LINE__, context);

  if (!p_context || !p_context->p_decoder) {
    res = ErrorNoContext;
    goto EXIT;
  }

  eof = p_context->p_decoder->is_eof();

  res = 0;
EXIT:
  if (res) {
    eof = 0;
  }

  LOGD("%s %d X res=%d \n", __func__, __LINE__, res);

  return eof;
}

//format:
//0 -- RGBA8888
//1 -- YV12
//
//#define ENABLE_BENCHMARK 1
jint _JNI(nativeDecodeFrameV)(JNIEnv* env, jobject thiz, jint context, jlong timestamp, jbyteArray data, jint format, jint out_width, jint out_height, jint rotate_angle) {
  int res = -1;
  ffmpeg_adapter_context* p_context = (ffmpeg_adapter_context*)context;
  AVFrame* p_new_frame = NULL;
  jbyte* data_array = NULL;
  jsize data_length = 0;
  uint64_t benchmark_timestamp = 0;
  long target_temp_buffer_length = 0;
  uint8_t* p_data = NULL;

  LOGD("%s %d E context=%d \n", __func__, __LINE__, context);
  if (!p_context || !p_context->p_decoder || !p_context->p_decoder->is_started()) {
    res = ErrorNoContext;
    goto EXIT;
  }

  data_array = getByteArray(env, data);
  data_length = getByteArrayLength(env, data);

  if (!data_array || !data_length ||
    (!format && data_length != ((out_width * out_height) << 2)) ||
    (format && data_length != ((out_width * out_height * 3) >> 1))) {
    res = ErrorParametersInvalid;
    goto EXIT;
  }

  res = p_context->p_decoder->set_output_video_format(format == 0 || format == 1 ? (int)AV_PIX_FMT_YUV420P : -1);
  if (res) {
    goto EXIT;
  }

  /*
  res = p_context->p_decoder->seek_to(static_cast<long>(timestamp));
  if (res) {
  goto EXIT;
  }
  */
#ifdef ENABLE_BENCHMARK
  benchmark_timestamp = system_timestamp();
#endif //ENABLE_BENCHMARK

  LOGD("%s %d context=%d \n", __func__, __LINE__, context);

  //YUV420P ONLY -- frame format = 0
  p_new_frame = p_context->p_decoder->get_video_frame();
  if (!p_new_frame || !p_new_frame->data[0] /*|| p_new_frame->format != 0*/) {
    res = ErrorDecodeFailed;
    goto EXIT;
  }

  LOGD("%s %d context=%d \n", __func__, __LINE__, context);

  p_data = p_new_frame->data[0];

  dump_to_file(DUMP_PATH("file.yuv"), p_new_frame->data[0], p_new_frame->width * p_new_frame->height * 3 / 2 * sizeof(uint8_t));

  target_temp_buffer_length = ((out_height * out_width * 3) >> 1) * sizeof(uint8_t); //YUV420P ONLY
  if (p_context->decode_buffer_length != target_temp_buffer_length) {
    p_context->decode_buffer_length = target_temp_buffer_length;

    if (p_context->temp_decode_crop_buffer) {
      free(p_context->temp_decode_crop_buffer);
      p_context->temp_decode_crop_buffer = NULL;
    }

    if (p_context->temp_decode_rotate_buffer) {
      free(p_context->temp_decode_rotate_buffer);
      p_context->temp_decode_rotate_buffer = NULL;
    }
  }

#ifdef ENABLE_BENCHMARK
  LOGE("%s%d VideoFrame cost : %lld \n", __func__, __LINE__, system_timestamp() - benchmark_timestamp);
  benchmark_timestamp = system_timestamp();
#endif //ENABLE_BENCHMARK

  //crop -- use crop buffer -- By default, width > height, use the sub to calculate the region to crop
  if (!p_context->temp_decode_crop_buffer) {
    p_context->temp_decode_crop_buffer = (uint8_t*)calloc(1, p_context->decode_buffer_length);
  }
  if (!p_context->temp_decode_crop_buffer) {
    res = ErrorAllocFailed;
    goto EXIT;
  }

  if (out_width > p_new_frame->width || out_height > p_new_frame->height) {
    res = ErrorParametersInvalid;
    goto EXIT;
  } else if (out_width < p_new_frame->width || out_height < p_new_frame->height) {
    //need crop
    //crop the size out
    res = YUV420P_crop(p_data,
      p_context->temp_decode_crop_buffer,
      p_new_frame->width,
      p_new_frame->height,
      ((p_new_frame->width - out_width) >> 1),
      ((p_new_frame->height - out_height) >> 1),
      out_width,
      out_height);
    if (res) {
      goto EXIT;
    }

    p_data = p_context->temp_decode_crop_buffer;

    dump_to_file(DUMP_PATH("crop.yuv"), p_context->temp_decode_crop_buffer, p_context->decode_buffer_length);
  }

#ifdef ENABLE_BENCHMARK
  LOGE("%s%d Crop cost : %lld \n", __func__, __LINE__, system_timestamp() - benchmark_timestamp);
  benchmark_timestamp = system_timestamp();
#endif //ENABLE_BENCHMARK

  if (rotate_angle) {
    if (!p_context->temp_decode_rotate_buffer) {
      p_context->temp_decode_rotate_buffer = (uint8_t*)calloc(1, p_context->decode_buffer_length);
    }
    if (!p_context->temp_decode_rotate_buffer) {
      res = ErrorAllocFailed;
      goto EXIT;
    }

    //rotate 90
    res = YUV420P_rotate(p_data,
      p_context->temp_decode_rotate_buffer,
      rotate_angle,
      out_width,
      out_height);
    if (res) {
      goto EXIT;
    }

    p_data = p_context->temp_decode_rotate_buffer;

    dump_to_file(DUMP_PATH("rotate.yuv"), p_context->temp_decode_rotate_buffer, p_context->decode_buffer_length);
  }

#ifdef ENABLE_BENCHMARK
  LOGE("%s%d Rotate cost : %lld \n", __func__, __LINE__, system_timestamp() - benchmark_timestamp);
#endif //ENABLE_BENCHMARK

  LOGD("%s %d context=%d \n", __func__, __LINE__, context);

  if (format) {
    //YUV case, no need to convert YUV->RGV 
    memcpy(data_array, p_data, p_context->decode_buffer_length);
    res = 0;
  } else {
#ifdef ENABLE_BENCHMARK
    benchmark_timestamp = system_timestamp();
#endif //ENABLE_BENCHMARK

    //yuv->rgba -- use java buffer
    res = ffmpeg_adapter_converter::convert_yv21_to_rgba(p_data, (uint8_t*)data_array, p_context->width, p_context->height);

#ifdef ENABLE_BENCHMARK
    LOGE("%s%d YV12->RGBA cost : %lld \n", __func__, __LINE__, system_timestamp() - benchmark_timestamp);
#endif //ENABLE_BENCHMARK
  }

  LOGD("%s %d context=%d \n", __func__, __LINE__, context);

  res = 0;
EXIT:
  if (res) {

  }

  if (!format) {
    dump_to_file(DUMP_PATH("camera.rgb"), (uint8_t*)data_array, data_length);
  }

  //memset(p_context->temp_decode_crop_buffer, 0, p_context->decode_buffer_length);
  //res = ffmpeg_adapter_converter::convert_rgba_to_yv21(data_array, p_context->temp_decode_crop_buffer, out_width, out_width);
  //dump_to_file(DUMP_PATH("X_rotate.yuv"), p_context->temp_decode_crop_buffer, p_context->decode_buffer_length);

  if (data_array) {
    releaseByteArray(env, data, data_array);
  }

  //depends on the OWN flag in ffmpeg_adapter_converter
  /*if (p_new_frame) {
    RELEASE_FRAME(p_new_frame);
    p_new_frame = NULL;
    }*/

  if (p_context) {
    p_context->last_error = res;
  }

  LOGD("%s %d X context=%d res=%d \n", __func__, __LINE__, context, res);

  return res;
}

jint _JNI(nativeDecodeFrameA)(JNIEnv* env, jobject thiz, jint context, jlong timestamp, jbyteArray data, jint format, jint volumn_ratio) {
  int res = -1;
  ffmpeg_adapter_context* p_context = (ffmpeg_adapter_context*)context;
  AVFrame* p_new_frame = NULL;
  jbyte* data_array = NULL;
  jsize data_length = 0;
  int uv_linesize = 0;
  int uv_offset = 0;
  uint64_t benchmark_timestamp = 0;
  int adjusted_format = 0;

  LOGD("%s %d E context=%d \n", __func__, __LINE__, context);
  if (!p_context || !p_context->p_decoder || !p_context->p_decoder->is_started()) {
    res = ErrorNoContext;
    goto EXIT;
  }

  data_array = getByteArray(env, data);
  data_length = getByteArrayLength(env, data);

  if (!data_array || data_length <= 0 || data_length < p_context->p_decoder->get_output_audio_buffer_size()) {
    res = ErrorParametersInvalid;
    goto EXIT;
  }

  //format -- map
  switch (format) {
  case 0:
    adjusted_format = (int)AV_SAMPLE_FMT_S16;
    break;
  case 1:
    adjusted_format = (int)AV_SAMPLE_FMT_U8;
    break;
  case 2:
    adjusted_format = (int)AV_SAMPLE_FMT_S32;
    break;
  default:
    break;
  }

  res = p_context->p_decoder->set_output_audio_format(adjusted_format);
  if (res) {
    goto EXIT;
  }

  /*
  res = p_context->p_decoder->seek_to(static_cast<long>(timestamp));
  if (res) {
  goto EXIT;
  }
  */

  p_new_frame = p_context->p_decoder->get_audio_frame();
  if (!p_new_frame || !p_new_frame->data[0] /*|| p_new_frame->format != 0*/) {
    res = ErrorDecodeFailed;
    goto EXIT;
  }

  if (volumn_ratio != 100) {
    res = scale_audio_frame_volume(p_new_frame, (double)volumn_ratio / 100);
    if (res)
      goto EXIT;
  }

  memset(data_array, 0, data_length);
  memcpy(data_array, p_new_frame->data[0], p_new_frame->linesize[0] * sizeof(uint8_t));

  res = 0;
EXIT:
  if (res) {

  }

  if (data_array) {
    releaseByteArray(env, data, data_array);
  }

  if (p_context) {
    p_context->last_error = res;
  }

  if (p_new_frame) {
	  if (NULL == p_new_frame->buf[0] && NULL != p_new_frame->data[0]){
		  av_free(p_new_frame->data[0]);
		  p_new_frame->data[0] = NULL;
	  }
    RELEASE_FRAME(p_new_frame);
    p_new_frame = NULL;
  }

  LOGD("%s %d X res=%d \n", __func__, __LINE__, res);

  return res;
}

jlong _JNI(nativeDecodeActualTimestampV)(JNIEnv* env, jobject thiz, jint context) {
  int res = -1;
  ffmpeg_adapter_context* p_context = (ffmpeg_adapter_context*)context;
  long timestamp = 0;

  LOGD("%s %d E context=%d \n", __func__, __LINE__, context);

  if (!p_context || !p_context->p_decoder) {
    res = ErrorNoContext;
    goto EXIT;
  }

  timestamp = p_context->p_decoder->get_video_timestamp();

  res = 0;
EXIT:
  if (res) {
    timestamp = 0;
  }

  LOGD("%s %d X context=%d timestamp=%ld \n", __func__, __LINE__, context, timestamp);

  return static_cast<long>(timestamp);
}

jlong _JNI(nativeDecodeActualTimestampA)(JNIEnv* env, jobject thiz, jint context) {
  int res = -1;
  ffmpeg_adapter_context* p_context = (ffmpeg_adapter_context*)context;
  long timestamp = 0;

  LOGD("%s %d E context=%d \n", __func__, __LINE__, context);

  if (!p_context || !p_context->p_decoder) {
    res = ErrorNoContext;
    goto EXIT;
  }

  timestamp = p_context->p_decoder->get_audio_timestamp();

  res = 0;
EXIT:
  if (res) {
    timestamp = 0;
  }

  LOGD("%s %d X context=%d timestamp=%ld \n", __func__, __LINE__, context, timestamp);

  return static_cast<long>(timestamp);
}


#ifdef __ANDROID__

jint _JNI(nativeEncodeFrameV)(JNIEnv* env, jobject thiz,
  jint context,
  jlong timestamp,
  jobject bitmap,
  jboolean with_audio) {
  int res = -1;
  ffmpeg_adapter_context* p_context = (ffmpeg_adapter_context*)context;
  bool locked = false;
  AVFrame* p_new_frame = NULL;

  AndroidBitmapInfo bitmapinfo;
  void* pixel_buf;

  LOGD("%s %d E context=%d ", __func__, __LINE__, context);
  if (!p_context || !p_context->p_encoder) {
    res = ErrorNoContext;
    goto EXIT;
  }

  res = AndroidBitmap_getInfo(env, bitmap, &bitmapinfo);
  if (res) {
    LOGE("AndroidBitmap_getInfo() failed ! error=%d", res);
    goto EXIT;
  }

  LOGI(
    "bitmap :: width is %d; height is %d; stride is %d; format is %d;flags is%d",
    bitmapinfo.width, bitmapinfo.height, bitmapinfo.stride, bitmapinfo.format,
    bitmapinfo.flags);

  //XT912 case -- get bitmap info succeed but the width is 2560 (4 times of actual width) and the format changed to alpha_8
  if (bitmapinfo.format != ANDROID_BITMAP_FORMAT_RGBA_8888 && bitmapinfo.width == 640 * 4) {
    bitmapinfo.width = 640;
    bitmapinfo.height = 640;
    bitmapinfo.format = ANDROID_BITMAP_FORMAT_RGBA_8888;
  }

  //      SkBitmap* bm = GraphicsJNI::getNativeBitmap(env, jbitmap);
  //      if (NULL == bm) {
  //          return ANDROID_BITMAP_RESULT_JNI_EXCEPTION;
  //      }
  //
  //      if (info) {
  //          info->width     = bm->width();
  //          info->height    = bm->height();
  //          info->stride    = bm->rowBytes();
  //          info->flags     = 0;
  //
  //          switch (bm->config()) {
  //              case SkBitmap::kARGB_8888_Config:
  //                  info->format = ANDROID_BITMAP_FORMAT_RGBA_8888;
  //                  break;
  //              case SkBitmap::kRGB_565_Config:
  //                  info->format = ANDROID_BITMAP_FORMAT_RGB_565;
  //                  break;
  //              case SkBitmap::kARGB_4444_Config:
  //                  info->format = ANDROID_BITMAP_FORMAT_RGBA_4444;
  //                  break;
  //              case SkBitmap::kA8_Config:
  //                  info->format = ANDROID_BITMAP_FORMAT_A_8;
  //                  break;
  //              default:
  //                  info->format = ANDROID_BITMAP_FORMAT_NONE;
  //                  break;
  //          }
  //      }
  //      return ANDROID_BITMAP_RESUT_SUCCESS;

  if (bitmapinfo.format != ANDROID_BITMAP_FORMAT_RGBA_8888) {
    LOGE("Bitmap format is not RGBA_8888");
    res = ErrorBitmapFormat;
    goto EXIT;
  }

  res = AndroidBitmap_lockPixels(env, bitmap, &pixel_buf);
  if (res) {
    LOGE("AndroidBitmap_lockPixels() failed ! error=%d", res);
    goto EXIT;
  }

  locked = true;

  p_new_frame = ffmpeg_adapter_encoder::create_ffmpeg_video_frame(AV_PIX_FMT_BGR32, //android bitmap format
    (uint8_t*)pixel_buf,
    bitmapinfo.width, bitmapinfo.height);
  if (!p_new_frame) {
    LOGE("Cannot create frame\n");
    res = ErrorAllocFailed;
    goto EXIT;
  }

  res = p_context->p_encoder->add_video_frame(p_new_frame, timestamp);
  if (res) {
    goto EXIT;
  }

  if(with_audio) {
    res = fill_audio(p_context);
    if(res) {
      goto EXIT;
    }
  }

  res = 0;
EXIT:
  if (res) {

  }

  if (p_new_frame) {
    RELEASE_FRAME(p_new_frame);
    p_new_frame = NULL;
  }

  if (locked) {
    AndroidBitmap_unlockPixels(env, bitmap);
  }

  if (p_context) {
    p_context->last_error = res;
  }

  LOGD("%s %d X res=%d ", __func__, __LINE__, res);

  return res;
}

#endif //__ANDROID__ //temporary disable this function -- per the complicated android bitmap (can replace it with other bitmap of Win32)

jint _JNI(nativeEncodeBufferV)(JNIEnv* env, jobject thiz, jint context, jlong timestamp, jbyteArray data, jint format, jint in_width, jint in_height, jint rotation_angle) {
  int res = -1;
  ffmpeg_adapter_context* p_context = (ffmpeg_adapter_context*)context;
  AVFrame* p_new_frame = NULL;
  jbyte* data_array = NULL;
  jsize data_length = 0;
  uint64_t benchmark_timestamp = 0;
  int adjusted_format = 0;
  long target_buffer_length = 0;

  LOGD("%s %d E context=%d format=%d in_width=%d in_height=%d angle=%d ", __func__, __LINE__, context, format, in_width, in_height, rotation_angle);
  if (!p_context || !p_context->p_encoder) {
    res = ErrorNoContext;
    goto EXIT;
  }

  data_array = getByteArray(env, data);
  data_length = getByteArrayLength(env, data);

  if (!data_array || data_length != ((in_width * in_height * 3) >> 1)) {
    res = ErrorParametersInvalid;
    goto EXIT;
  }

  target_buffer_length = ((p_context->width * p_context->height * 3) >> 1) * sizeof(uint8_t);
  if (p_context->encode_buffer_length != target_buffer_length) {
    p_context->encode_buffer_length = target_buffer_length; //YUV420P ONLY

    if (!p_context->temp_encode_buffer) {
      free(p_context->temp_encode_buffer);
      p_context->temp_encode_buffer = NULL;
    }
  }

  //crop -- use crop buffer -- By default, width > height, use the sub to calculate the region to crop
  if (!p_context->temp_encode_buffer) {
    p_context->temp_encode_buffer = (uint8_t*)calloc(1, p_context->encode_buffer_length);
  }
  if (!p_context->temp_encode_buffer) {
    res = ErrorAllocFailed;
    goto EXIT;
  }

  //TODO -- support to crop height
  if (p_context->width < in_width || p_context->height < in_height) {
    res = YUV420SP_crop((uint8_t*)data_array,
      p_context->temp_encode_buffer,
      in_width, in_height,
      ((in_width - p_context->width) >> 1), ((in_height - p_context->height) >> 1),
      p_context->width, p_context->height);
    if (res)
      goto EXIT;
  }

  dump_to_file("/sdcard/crop.yuv", p_context->temp_encode_buffer, p_context->encode_buffer_length);

  //TODO -- format
  p_new_frame = ffmpeg_adapter_encoder::create_ffmpeg_video_frame(AV_PIX_FMT_NV21, //android bitmap format
    p_context->width, p_context->height);
  if (!p_new_frame) {
    res = ErrorAllocFailed;
    goto EXIT;
  }

  if (rotation_angle) {
    res = YUV420SP_rotate(p_context->temp_encode_buffer, p_new_frame->data[0], rotation_angle, p_context->width, p_context->height);
    if (res)
      goto EXIT;
  }

  res = p_context->p_encoder->add_video_frame(p_new_frame, static_cast<long>(timestamp));
  if (res) {
    goto EXIT;
  }

  res = 0;
EXIT:
  if (res) {

  }

  if (data_array) {
    releaseByteArray(env, data, data_array);
  }

  if (p_new_frame) {
    RELEASE_FRAME(p_new_frame);
    p_new_frame = NULL;
  }

  if (p_context) {
    p_context->last_error = res;
  }

  LOGD("%s %d X res=%d ", __func__, __LINE__, res);

  return res;
}

jint _JNI(nativeEncodeBufferA)(JNIEnv* env, jobject thiz, jint context, jlong timestamp, jbyteArray data, jint format, jint size) {
  int res = -1;
  ffmpeg_adapter_context* p_context = (ffmpeg_adapter_context*)context;
  AVFrame* p_new_frame = NULL;
  jbyte* data_array = NULL;
  jsize data_length = 0;

  LOGD("%s %d E context=%d size=%d", __func__, __LINE__, context, size);

  if (!p_context || !p_context->p_encoder) {
    res = ErrorNoContext;
    goto EXIT;
  }

  data_array = getByteArray(env, data);
  data_length = getByteArrayLength(env, data);

  if (!data_array || data_length < 4) {
    res = ErrorParametersInvalid;
    goto EXIT;
  }

  //TODO -- to modify the sample rate and channels.
  p_new_frame = ffmpeg_adapter_encoder::create_ffmpeg_audio_frame(
    (size >> 2), //TODO -- to calculate smarter
    (int)AV_SAMPLE_FMT_S16,
    2,
    AV_CH_LAYOUT_STEREO,
    DEFAULT_AUDIO_SAMPLE_RATE, (uint8_t*)data_array, size);
  if (!p_new_frame || !p_new_frame->data[0]) {
    res = ErrorAllocFailed;
    goto EXIT;
  }

  res = p_context->p_encoder->add_audio_frame(p_new_frame, static_cast<long>(timestamp));
  if (res) {
    goto EXIT;
  }

  res = 0;
EXIT:
  if (res) {

  }

  if (data_array) {
    releaseByteArray(env, data, data_array);
  }

  if (p_new_frame) {
    RELEASE_FRAME(p_new_frame);
    p_new_frame = NULL;
  }

  if (p_context) {
    p_context->last_error = res;
  }

  LOGD("%s %d X res=%d ", __func__, __LINE__, res);

  return res;
}

jint _JNI(nativeAddCompressedFrameV)(JNIEnv* env, jobject thiz, jint context, jlong timestamp, jbyteArray data, jint offset, jint size, jint flag) {
  int res = -1;
  ffmpeg_adapter_context* p_context = (ffmpeg_adapter_context*)context;
  jbyte* data_array = NULL;
  jsize data_length = 0;;

  LOGD("%s %d E context=%d size=%d flag=%d", __func__, __LINE__, context, size, flag);
  if (!p_context || !p_context->p_encoder) {
    res = ErrorNoContext;
    goto EXIT;
  }

  data_array = getByteArray(env, data);
  data_length = getByteArrayLength(env, data);

  if (!data_array || data_length <= 0 || data_length <= offset || offset < 0 || size <= 0) {
    res = ErrorParametersInvalid;
    goto EXIT;
  }

  res = p_context->p_encoder->add_compressed_video_frame((uint8_t*)(data_array + offset), size, timestamp, flag);
  if (res) {
    goto EXIT;
  }

  res = 0;
EXIT:
  if (res) {

  }

  if (data_array) {
    releaseByteArray(env, data, data_array);
  }

  if (p_context) {
    p_context->last_error = res;
  }

  LOGD("%s %d X res=%d ", __func__, __LINE__, res);

  return res;
}

jint _JNI(nativeAddExtraDataV)(JNIEnv* env, jobject thiz, jint context, jbyteArray data, jint offset, jint size) {
  int res = -1;
  ffmpeg_adapter_context* p_context = (ffmpeg_adapter_context*)context;
  jbyte* data_array = NULL;
  jsize data_length = 0;;

  LOGD("%s %d E context=%d size=%d ", __func__, __LINE__, context, size);
  if (!p_context || !p_context->p_encoder) {
    res = ErrorNoContext;
    goto EXIT;
  }

  data_array = getByteArray(env, data);
  data_length = getByteArrayLength(env, data);

  if (!data_array || data_length <= 0 || data_length <= offset || offset < 0 || size <= 0) {
    res = ErrorParametersInvalid;
    goto EXIT;
  }

  res = p_context->p_encoder->set_video_extra_data((uint8_t*)(data_array + offset), size);
  if (res) {
    goto EXIT;
  }

  res = 0;
EXIT:
  if (res) {

  }

  if (data_array) {
    releaseByteArray(env, data, data_array);
  }


  if (p_context) {
    p_context->last_error = res;
  }

  LOGD("%s %d X res=%d ", __func__, __LINE__, res);

  return res;
}

jint _JNI(nativeAddCompressedFrameA)(JNIEnv* env, jobject thiz, jint context, jlong timestamp, jbyteArray data, jint offset, jint size, jint flag) {
  int res = -1;
  ffmpeg_adapter_context* p_context = (ffmpeg_adapter_context*)context;
  jbyte* data_array = NULL;
  jsize data_length = 0;;

  LOGD("%s %d E context=%d size=%d flag=%d", __func__, __LINE__, context, size, flag);
  if (!p_context || !p_context->p_encoder) {
    res = ErrorNoContext;
    goto EXIT;
  }

  data_array = getByteArray(env, data);
  data_length = getByteArrayLength(env, data);

  if (!data_array || data_length <= 0 || data_length <= offset || offset < 0 || size <= 0) {
    res = ErrorParametersInvalid;
    goto EXIT;
  }

  res = p_context->p_encoder->add_compressed_audio_frame((uint8_t*)(data_array + offset), size, timestamp);
  if (res) {
    goto EXIT;
  }

  res = 0;
EXIT:
  if (res) {

  }

  if (data_array) {
    releaseByteArray(env, data, data_array);
  }

  if (p_context) {
    p_context->last_error = res;
  }

  LOGD("%s %d X res=%d ", __func__, __LINE__, res);

  return res;
}

jint _JNI(nativeAddExtraDataA)(JNIEnv* env, jobject thiz, jint context, jbyteArray data, jint offset, jint size) {
  int res = -1;
  ffmpeg_adapter_context* p_context = (ffmpeg_adapter_context*)context;
  jbyte* data_array = NULL;
  jsize data_length = 0;;

  LOGD("%s %d E context=%d size=%d ", __func__, __LINE__, context, size);
  if (!p_context || !p_context->p_encoder) {
    res = ErrorNoContext;
    goto EXIT;
  }

  data_array = getByteArray(env, data);
  data_length = getByteArrayLength(env, data);

  if (!data_array || data_length <= 0 || data_length <= offset || offset < 0 || size <= 0) {
    res = ErrorParametersInvalid;
    goto EXIT;
  }

  res = p_context->p_encoder->set_audio_extra_data((uint8_t*)(data_array + offset), size);
  if (res) {
    goto EXIT;
  }

  res = 0;
EXIT:
  if (res) {

  }

  if (data_array) {
    releaseByteArray(env, data, data_array);
  }


  if (p_context) {
    p_context->last_error = res;
  }

  LOGD("%s %d X res=%d ", __func__, __LINE__, res);

  return res;
}

#ifdef __ANDROID__

jint _JNI(nativeRgbaBitmapToRgbaBuffer)(JNIEnv* env, jobject thiz, jint context, jobject bitmap, jbyteArray rgbadata) {
  int res = -1;
  ffmpeg_adapter_context* p_context = (ffmpeg_adapter_context*)context;
  bool locked = false;
  AndroidBitmapInfo bitmapinfo;
  void* rgba_pixel_buf = NULL;
  jbyte* data_array = NULL;
  jsize data_length = 0;

  LOGD("%s %d E context=%d ", __func__, __LINE__, context);
  if (!p_context) {
    res = ErrorNoContext;
    goto EXIT;
  }

  res = AndroidBitmap_getInfo(env, bitmap, &bitmapinfo);
  if (res) {
    LOGE("AndroidBitmap_getInfo() failed ! error=%d", res);
    goto EXIT;
  }

  LOGI(
    "bitmap :: width is %d; height is %d; stride is %d; format is %d;flags is%d",
    bitmapinfo.width, bitmapinfo.height, bitmapinfo.stride, bitmapinfo.format,
    bitmapinfo.flags);

  //XT912 case -- get bitmap info succeed but the width is 2560 (4 times of actual width) and the format changed to alpha_8
  if (bitmapinfo.format != ANDROID_BITMAP_FORMAT_RGBA_8888 && bitmapinfo.width == 640 * 4) {
    bitmapinfo.width = 640;
    bitmapinfo.height = 640;
    bitmapinfo.format = ANDROID_BITMAP_FORMAT_RGBA_8888;
  }

  if (bitmapinfo.format != ANDROID_BITMAP_FORMAT_RGBA_8888) {
    LOGE("Bitmap format is not RGBA_8888");
    res = ErrorBitmapFormat;
    goto EXIT;
  }

  res = AndroidBitmap_lockPixels(env, bitmap, &rgba_pixel_buf);
  if (res) {
    LOGE("AndroidBitmap_lockPixels() failed ! error=%d", res);
    goto EXIT;
  }

  locked = true;

  data_array = getByteArray(env, rgbadata);
  data_length = getByteArrayLength(env, rgbadata);

  if (!data_array || data_length <= 0 ||
    data_length != ((bitmapinfo.width * bitmapinfo.height) << 2)) {
    res = ErrorParametersInvalid;
    goto EXIT;
  }

  memcpy((uint8_t*)data_array, rgba_pixel_buf, data_length);

  res = 0;
EXIT:
  if (res) {

  }

  if (locked) {
    AndroidBitmap_unlockPixels(env, bitmap);
  }

  if (data_array) {
    releaseByteArray(env, rgbadata, data_array);
  }

  if (p_context) {
    p_context->last_error = res;
  }

  LOGD("%s %d X res=%d ", __func__, __LINE__, res);

  return res;
}

jint _JNI(nativeRgbaBitmapToYuv)(JNIEnv* env, jobject thiz, jint context, jobject bitmap, jbyteArray yuvdata, int flag) {
  int res = -1;
  ffmpeg_adapter_context* p_context = (ffmpeg_adapter_context*)context;
  bool locked = false;
  AndroidBitmapInfo bitmapinfo;
  void* rgba_pixel_buf = NULL;
  jbyte* yuv_data_array = NULL;
  jsize yuv_data_length = 0;
  long y_size = 0;

  LOGD("%s %d E context=%d ", __func__, __LINE__, context);
  if (!p_context) {
    res = ErrorNoContext;
    goto EXIT;
  }

  res = AndroidBitmap_getInfo(env, bitmap, &bitmapinfo);
  if (res) {
    LOGE("AndroidBitmap_getInfo() failed ! error=%d", res);
    goto EXIT;
  }

  LOGI(
    "bitmap :: width is %d; height is %d; stride is %d; format is %d;flags is%d",
    bitmapinfo.width, bitmapinfo.height, bitmapinfo.stride, bitmapinfo.format,
    bitmapinfo.flags);

  //XT912 case -- get bitmap info succeed but the width is 2560 (4 times of actual width) and the format changed to alpha_8
  if (bitmapinfo.format != ANDROID_BITMAP_FORMAT_RGBA_8888 && bitmapinfo.width == 640 * 4) {
    bitmapinfo.width = 640;
    bitmapinfo.height = 640;
    bitmapinfo.format = ANDROID_BITMAP_FORMAT_RGBA_8888;
  }

  if (bitmapinfo.format != ANDROID_BITMAP_FORMAT_RGBA_8888) {
    LOGE("Bitmap format is not RGBA_8888");
    res = ErrorBitmapFormat;
    goto EXIT;
  }

  y_size = bitmapinfo.width * bitmapinfo.height;

  res = AndroidBitmap_lockPixels(env, bitmap, &rgba_pixel_buf);
  if (res) {
    LOGE("AndroidBitmap_lockPixels() failed ! error=%d", res);
    goto EXIT;
  }

  locked = true;

  yuv_data_array = getByteArray(env, yuvdata);
  yuv_data_length = getByteArrayLength(env, yuvdata);

  if (!yuv_data_array || yuv_data_length <= 0 ||
    yuv_data_length != (y_size + (y_size >> 1))) {
    res = ErrorParametersInvalid;
    goto EXIT;
  }

  res = convert_rgba_to_yuv(
    (uint8_t*)rgba_pixel_buf, 
    (uint8_t*)yuv_data_array, 
    bitmapinfo.width, 
    bitmapinfo.height,
    flag);
  if (res)
    goto EXIT;

  res = 0;
EXIT:
  if (res) {

  }

  if (locked) {
    AndroidBitmap_unlockPixels(env, bitmap);
  }

  if (yuv_data_array) {
    releaseByteArray(env, yuvdata, yuv_data_array);
  }

  if (p_context) {
    p_context->last_error = res;
  }

  LOGD("%s %d X res=%d ", __func__, __LINE__, res);

  return res;
}

#endif //---------

jint _JNI(nativeRgbaToYuv)(JNIEnv* env, jobject thiz, jint context, jbyteArray rgbadata, jbyteArray yuvdata, jint width, jint height, int flag) {
  int res = -1;

  jbyte* rgba_data_array = NULL;
  jsize rgba_data_length = 0;
  jbyte* yuv_data_array = NULL;
  jsize yuv_data_length = 0;
  int y_size = width * height;

  if (width <= 0 || height <= 0) {
    res = ErrorParametersInvalid;
    goto EXIT;
  }

  rgba_data_array = getByteArray(env, rgbadata);
  rgba_data_length = getByteArrayLength(env, rgbadata);

  yuv_data_array = getByteArray(env, yuvdata);
  yuv_data_length = getByteArrayLength(env, yuvdata);

  if (!rgba_data_array || rgba_data_length <= 0 ||
    !yuv_data_array || yuv_data_length <= 0 ||
    rgba_data_length != (y_size << 2) ||
    yuv_data_length != (y_size + (y_size >> 1))) {
    res = ErrorParametersInvalid;
    goto EXIT;
  }

  res = convert_rgba_to_yuv(
    (uint8_t*)rgba_data_array,
    (uint8_t*)yuv_data_array,
    width,
    height,
    flag);
  if (res)
    goto EXIT;

  res = 0;
EXIT:
  if (res) {

  }

  if (rgba_data_array) {
    releaseByteArray(env, rgbadata, rgba_data_array);
  }

  if (yuv_data_array) {
    releaseByteArray(env, yuvdata, yuv_data_array);
  }

  return res;
}

static void native_ffmpeg_adapter_end(ffmpeg_adapter_context* p_context) {
  if (p_context) {
    if (p_context->p_encoder) {
      //we need this -- be care of the last UP-scale volume part! (if up)
      //to finish the last output, do not care about the result
      if (p_context->p_encoder->is_started() && !p_context->last_error) {
        p_context->p_encoder->add_audio_frame(NULL);
        p_context->p_encoder->add_video_frame(NULL);
      }

      delete p_context->p_encoder;
      p_context->p_encoder = NULL;
    }

    if (p_context->p_decoder) {
      delete p_context->p_decoder;
      p_context->p_decoder = NULL;
    }

    if (p_context->temp_decode_crop_buffer) {
      free(p_context->temp_decode_crop_buffer);
      p_context->temp_decode_crop_buffer = NULL;
    }

    if (p_context->temp_decode_rotate_buffer) {
      free(p_context->temp_decode_rotate_buffer);
      p_context->temp_decode_rotate_buffer = NULL;
    }

    if (p_context->temp_encode_buffer) {
      free(p_context->temp_encode_buffer);
      p_context->temp_encode_buffer = NULL;
    }

    p_context->decode_buffer_length = 0;
    p_context->encode_buffer_length = 0;
    p_context->last_error = 0;
  }
}

static int convert_rgba_to_yuv(uint8_t* rgba, uint8_t* yuv, int width, int height, int flag) {
  switch (flag) {
  case 1:
    return ffmpeg_adapter_converter::convert_rgba_to_yv12(rgba, yuv, width, height);
    break;
  case 2:
    return ffmpeg_adapter_converter::convert_rgba_to_nv12(rgba, yuv, width, height);
    break;
  case 3:
    return ffmpeg_adapter_converter::convert_rgba_to_nv21(rgba, yuv, width, height);
    break;
  default:
    return ErrorParametersInvalid;
  }
}

static int fill_audio(ffmpeg_adapter_context* p_context) {
  int res = -1;
  AVFrame* p_temp_audio_frame = NULL;

  //Below codes probably (if no check on started) cause some weird problem... such as No VOICE...
  if (p_context->p_decoder && p_context->p_decoder->is_started()) {
    long audio_current_timestamp = 0;
    long rest_timestamp = 0;
    while ((audio_current_timestamp = p_context->p_decoder->get_audio_timestamp() - p_context->from_timestamp) <= p_context->p_encoder->get_current_video_duration())  {
      p_temp_audio_frame = p_context->p_decoder->get_audio_frame();
      if (p_temp_audio_frame) {
        LOGE("duration = %ld, current duration=%ld", p_context->duration, audio_current_timestamp);

        audio_current_timestamp = audio_current_timestamp < 0 ? 0 : audio_current_timestamp;

        rest_timestamp = p_context->duration - audio_current_timestamp;
        rest_timestamp = rest_timestamp > 0 ? rest_timestamp : 0;
        if ((p_context->duration > 6000000L) && (rest_timestamp < 3000000L)) {
          scale_audio_frame_volume(p_temp_audio_frame, (double)(rest_timestamp) / 3000000L);
        }

        res = p_context->p_encoder->add_audio_frame(p_temp_audio_frame);
        if (res)
          goto EXIT;
		if (NULL == p_temp_audio_frame->buf[0] && NULL != p_temp_audio_frame->data[0]){
			av_free(p_temp_audio_frame->data[0]);
			p_temp_audio_frame->data[0] = NULL;
		}
		
        RELEASE_FRAME(p_temp_audio_frame);
      } else {
        if (p_context->p_decoder->is_eof()) {
          p_context->p_encoder->add_audio_frame(NULL);
          res = 0;
          break;
        } else {
          //continue;
          break;
        }
      }
    }

    if (p_context->p_encoder->use_fake_audio()) {
      while (p_context->p_encoder->get_current_audio_duration() <= p_context->p_encoder->get_current_video_duration()) {
        p_context->p_encoder->add_audio_frame(NULL);
      }
    }
  }

  res = 0;
EXIT:
  if (p_temp_audio_frame) {
	  if (NULL == p_temp_audio_frame->buf && NULL != p_temp_audio_frame->data[0]){
		  av_free(p_temp_audio_frame->data[0]);
		  p_temp_audio_frame->data[0] = NULL;
	  }
    RELEASE_FRAME(p_temp_audio_frame);
    p_temp_audio_frame = NULL;
  }

  return res;
}

jint _JNI(nativeGetLastError)(JNIEnv* env, jobject thiz, jint context) {
  ffmpeg_adapter_context* p_context = (ffmpeg_adapter_context*)context;
  if (!p_context) {
    return ErrorNoGlobalInstance;
  }

  return p_context->last_error;
}


jint _JNI(nativeConcatFiles)(JNIEnv* env, jobject thiz, jobjectArray pathArray, jstring outputPath) {

#ifdef __ANDROID__
  int res = -1;
  jsize pathsCount = 0;
  char **paths = NULL;
  jstring *jpaths = NULL;
  const char *outputpath = NULL;

  if(!pathArray || !outputPath) {
    res = ErrorParametersInvalid;
    goto EXIT;
  }

  pathsCount = env->GetArrayLength(pathArray);

  paths = new char*[pathsCount];
  jpaths = new jstring[pathsCount];
  if(!paths || !jpaths) {
    res = ErrorAllocFailed;
    goto EXIT;
  }

  outputpath = getString(env, outputPath);

  for (int i = 0; i < pathsCount; i++) {
    jpaths[i] = (jstring)env->GetObjectArrayElement(pathArray, i);
    paths[i] = (char*)getString(env, jpaths[i]);
  }

  res = concat_files((const char **)paths, pathsCount, outputpath);
  if (res)
    goto EXIT;

  res = 0;
EXIT:
  if (res) {

  }

  if (outputpath) {
    releaseString(env, outputPath, outputpath);
  }

  for (int i = 0; i < pathsCount; i++) {
    if (paths[i]) {
      releaseString(env, jpaths[i], paths[i]);
    }
  }

  return res;
#else
  return 0;
#endif
}