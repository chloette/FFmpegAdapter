#ifndef __INCLUDE_FFMPAGE_ADAPTER_H__
#define __INCLUDE_FFMPAGE_ADAPTER_H__

#include "ffmpeg_adapter_common.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif 
  jint _JNI(nativeInit)(JNIEnv* env, jobject thiz);
  void _JNI(nativeRelease)(JNIEnv* env, jobject thiz, jint context);

  jint _JNI(nativeSetAudioStartFrom)(JNIEnv* env, jobject thiz, jint context, jlong from);
  jint _JNI(nativeSetEncodeDuration)(JNIEnv* env, jobject thiz, jint context, jlong duration);

  jint _JNI(nativeDecodeFrom)(JNIEnv* env, jobject thiz, jint context, jstring fileName);
  jint _JNI(nativeEncodeTo)(JNIEnv* env, jobject thiz, jint context, jstring fileName);

  jlong _JNI(nativeEncodeGetDurationV)(JNIEnv* env, jobject thiz, jint context);
  jlong _JNI(nativeEncodeGetDurationA)(JNIEnv* env, jobject thiz, jint context);
  
  jint _JNI(nativeEncodeSetResolution)(JNIEnv* env, jobject thiz, jint context, jint width, jint height);
  jint _JNI(nativeEncodeSetFps)(JNIEnv* env, jobject thiz, jint context, jdouble fps);
  jint _JNI(nativeEncodeSetVideoQuality)(JNIEnv* env, jobject thiz, jint context, jint video_quality);

  jlong _JNI(nativeDecodeGetDurationV)(JNIEnv* env, jobject thiz, jint context);
  jlong _JNI(nativeDecodeGetDurationA)(JNIEnv* env, jobject thiz, jint context);
  
  jint _JNI(nativeDecodeGetAudioFormat)(JNIEnv* env, jobject thiz, jint context);
  jint _JNI(nativeDecodeGetAudioSamplerate)(JNIEnv* env, jobject thiz, jint context);
  jint _JNI(nativeDecodeGetAudioChannels)(JNIEnv* env, jobject thiz, jint context);
  jint _JNI(nativeDecodeGetAudioChannelLayout)(JNIEnv* env, jobject thiz, jint context);
  jint _JNI(nativeDecodeGetAudioFrameSize)(JNIEnv* env, jobject thiz, jint context);
  jint _JNI(nativeDecodeGetAudioBufferSize)(JNIEnv* env, jobject thiz, jint context);

  jint _JNI(nativeDecodeSetAudioFormat)(JNIEnv* env, jobject thiz, jint context, jint format);
  jint _JNI(nativeDecodeSetAudioSamplerate)(JNIEnv* env, jobject thiz, jint context, jint samplerate);
  jint _JNI(nativeDecodeSetAudioChannels)(JNIEnv* env, jobject thiz, jint context, jint channels);
  jint _JNI(nativeDecodeSetAudioFrameSize)(JNIEnv* env, jobject thiz, jint context, jint framesize);

  jint _JNI(nativeDecodeEoF)(JNIEnv* env, jobject thiz, jint context);
  jint _JNI(nativeDecodeSeekTo)(JNIEnv* env, jobject thiz, jint context, jlong timestamp);
  
  jint _JNI(nativeDecodeFrameV)(JNIEnv* env, jobject thiz, jint context, jlong timestamp, jbyteArray data, jint format, jint out_width, jint out_height, jint rotate_angle);
  jint _JNI(nativeDecodeFrameA)(JNIEnv* env, jobject thiz, jint context, jlong timestamp, jbyteArray data, jint format, jint volumn_ratio);
  jlong _JNI(nativeDecodeActualTimestampV)(JNIEnv* env, jobject thiz, jint context);
  jlong _JNI(nativeDecodeActualTimestampA)(JNIEnv* env, jobject thiz, jint context);

  jint _JNI(nativeEncodeFrameV)(JNIEnv* env, jobject thiz, jint context, jlong timestamp, jobject bitmap, jboolean with_audio);
  jint _JNI(nativeEncodeBufferV)(JNIEnv* env, jobject thiz, jint context, jlong timestamp, jbyteArray data, jint format, jint in_width, jint in_height, jint rotation_angle);
  jint _JNI(nativeEncodeBufferA)(JNIEnv* env, jobject thiz, jint context, jlong timestamp, jbyteArray data, jint format, jint size);

  jint _JNI(nativeAddCompressedFrameV)(JNIEnv* env, jobject thiz, jint context, jlong timestamp, jbyteArray data, jint offset, jint size, jint flag);
  jint _JNI(nativeAddExtraDataV)(JNIEnv* env, jobject thiz, jint context, jbyteArray data, jint offset, jint size);
  jint _JNI(nativeAddCompressedFrameA)(JNIEnv* env, jobject thiz, jint context, jlong timestamp, jbyteArray data, jint offset, jint size, jint flag);
  jint _JNI(nativeAddExtraDataA)(JNIEnv* env, jobject thiz, jint context, jbyteArray data, jint offset, jint size);
  
  jint _JNI(nativeRgbaBitmapToRgbaBuffer)(JNIEnv* env, jobject thiz, jint context, jobject bitmap, jbyteArray yuvdata);
  jint _JNI(nativeRgbaBitmapToYuv)(JNIEnv* env, jobject thiz, jint context, jobject bitmap, jbyteArray yuvdata, jint flag);
  jint _JNI(nativeRgbaToYuv)(JNIEnv* env, jobject thiz, jint context, jbyteArray rgbadata, jbyteArray yuvdata, jint width, jint height, int flag);

  jint _JNI(nativeGetLastError)(JNIEnv* env, jobject thiz, jint context);

  jint _JNI(nativeConcatFiles)(JNIEnv* env, jobject thiz, jobjectArray pathArray, jstring outputPath);

#ifndef __ANDROID__

  //On WINDOWS, use Macro to avoid the useless "env" and "thiz".
#define _nativeInit() nativeInit(NULL, 0)
#define _nativeRelease(context) nativeRelease(NULL, 0, context)

#define _nativeSetAudioStartFrom(context, from) nativeSetAudioStartFrom(NULL, 0, context, from)
#define _nativeSetEncodeDuration(context, duration) nativeSetEncodeDuration(NULL, 0, context, duration)

#define _nativeDecodeFrom(context, path)  nativeDecodeFrom(NULL, 0, context, path)
#define _nativeEncodeTo(context, fileName)  nativeEncodeTo(NULL, 0, context, fileName)

#define _nativeEncodeGetDurationV(context) nativeEncodeGetDurationV(NULL, 0, context)
#define _nativeEncodeGetDurationA(context) nativeEncodeGetDurationA(NULL, 0, context)
#define _nativeEncodeSetResolution(context, width, height)  nativeEncodeSetResolution(NULL, 0, context, width, height)
#define _nativeEncodeSetFps(context, fps) nativeEncodeSetFps(NULL, 0, context, fps)
#define _nativeEncodeSetVideoQuality(context, quality) nativeEncodeSetVideoQuality(NULL, 0, context, quality)

#define _nativeDecodeGetDurationV(context) nativeDecodeGetDurationV(NULL, 0, context)
#define _nativeDecodeGetDurationA(context) nativeDecodeGetDurationA(NULL, 0, context)

#define _nativeDecodeGetAudioFormat(context) nativeDecodeGetAudioFormat(NULL, 0, context)
#define _nativeDecodeGetAudioSamplerate(context) nativeDecodeGetAudioSamplerate(NULL, 0, context)
#define _nativeDecodeGetAudioChannels(context) nativeDecodeGetAudioChannels(NULL, 0, context)
#define _nativeDecodeGetAudioChannelLayout(context) nativeDecodeGetAudioChannelLayout(NULL, 0, context)
#define _nativeDecodeGetAudioFrameSize(context) nativeDecodeGetAudioFrameSize(NULL, 0, context)
#define _nativeDecodeGetAudioBufferSize(context) nativeDecodeGetAudioBufferSize(NULL, 0, context)

#define _nativeDecodeSetAudioFormat(context, format) nativeDecodeSetAudioFormat(NULL, 0, context, format)
#define _nativeDecodeSetAudioSamplerate(context, samplerate) nativeDecodeSetAudioSamplerate(NULL, 0, context, samplerate)
#define _nativeDecodeSetAudioChannels(context, channels) nativeDecodeSetAudioChannels(NULL, 0, context, channels)
#define _nativeDecodeSetAudioFrameSize(context, framesize) nativeDecodeSetAudioFrameSize(NULL, 0, context, framesize)


#define _nativeDecodeEoF(context) nativeDecodeEoF(NULL, 0, context)
#define _nativeDecodeSeekTo(context, timestamp) nativeDecodeSeekTo(NULL, 0, context, timestamp)
#define _nativeDecodeFrameV(context, timestamp, data, format, out_width, out_height, rotate_angle) nativeDecodeFrameV(NULL, 0, context, timestamp, data, format, out_width, out_height, rotate_angle)
#define _nativeDecodeFrameA(context, timestamp, data, format, volumn_ratio) nativeDecodeFrameA(NULL, 0, context, timestamp, data, format, volumn_ratio)
#define _nativeDecodeActualTimestampV(context) nativeDecodeActualTimestampV(NULL, 0, context)
#define _nativeDecodeActualTimestampA(context) nativeDecodeActualTimestampA(NULL, 0, context)


#define _nativeEncodeFrameV(context, timestamp, bitmap, with_audio) nativeEncodeFrameV(NULL, 0, context, timestamp, bitmap, with_audio)
#define _nativeEncodeBufferV(context, timestamp, data, format, in_width, in_height, rotate_angle) nativeEncodeBufferV(NULL, 0, context, timestamp, data, format, in_width, in_height, rotate_angle)
#define _nativeEncodeBufferA(context, timestamp, data, format, size) nativeEncodeBufferA(NULL, 0, context, timestamp, data, format, size)

#define _nativeAddCompressedFrameV(context, timestamp, data, offset, size, flag) nativeAddCompressedFrameV(NULL, 0, context, timestamp, data, offset, size, flag)
#define _nativeAddExtraDataV(context, timestamp, data, offset, size) nativeAddExtraDataV(NULL, 0, context, timestamp, data, offset, size)
#define _nativeAddCompressedFrameA(context, timestamp, data, offset, size, flag) nativeAddCompressedFrameA(NULL, 0, context, timestamp, data, offset, size, flag)
#define _nativeAddExtraDataA(context, timestamp, data, offset, size) nativeAddExtraDataA(NULL, 0, context, timestamp, data, offset, size)

#define _nativeRgbaBitmapToRgbaBuffer(context, rgbabitmap, rgbadata) nativeRgbaBitmapToRgbaBuffer(NULL, 0, context, rgbabitmap, rgbadata)
#define _nativeRgbaBitmapToYuv(context, rgbabitmap, yuvdata, flag) nativeRgbaToYuv(NULL, 0, context, rgbabitmap, yuvdata, flag)  
#define _nativeRgbaToYuv(context, rgbadata, yuvdata, width, height, flag) nativeRgbaToYuv(NULL, 0, context, rgbadata, yuvdata, width, height, flag)  

#define _nativeGetLastError(context) nativeGetLastError(NULL, 0, context)

#endif  //__ANDROID__

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif  //__cplusplus__

#endif //__INCLUDE_FFMPAGE_ADAPTER_H__
