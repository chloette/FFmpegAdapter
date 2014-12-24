#ifndef __INCLUDE_FFMPEG_ADAPTER_COMMON_H__
#define __INCLUDE_FFMPEG_ADAPTER_COMMON_H__

#if defined(WIN32) && !defined(__cplusplus)
#define inline __inline
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __ANDROID__
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <windows.h>
#else
//android log here
#include <android/log.h>
#include <fcntl.h>
#endif//__ANDROID__

#include <avcodec.h>
#include <avformat.h>
#include <swscale.h>
#include <swresample.h>
#include <error.h>
#include <audio_fifo.h>
#include "ffmpeg_adapter_util.h"
#include "opt.h"
#include "ffmpeg_adapter_transform.h"
#include "ffmpeg_adapter_rotate.h"
#include "ffmpeg_adapter_crop.h"

#ifdef __cplusplus
}
#endif

#ifdef __ANDROID__

  #define  _JNI(x)  Java_com_android_ffmpegadapter_FFmpegAdapter_##x //on Android.. :|

#else

  #define  _JNI(x)  x //on win32/64, just use the totally same api like Java

  typedef int jint;
  typedef int32_t JNIEnv;
  typedef uint8_t jobject;
  typedef const char* jstring;
  typedef double jdouble;
  typedef long jlong;
  typedef uint8_t jbyte;
  typedef long jsize;
  typedef int jboolean;

  typedef struct _jni_jshortArray_t {
    uint16_t* data;
    long length;
  } jshortArray;

  typedef struct _jni_jbyteArray_t {
    uint8_t* data;
    long length;
  } jbyteArray;

  typedef struct _jni_jobjectArray_t {
	  void** data;
	  long length;
  } jobjectArray;

#endif  //__ANDROID__

#ifndef __ANDROID__
  //standard log here
  #if __STDC_VERSION__ < 199901L  
  # if __GNUC__ >= 2  
  #  define __func__ __FUNCTION__  
  # else  
  #  define __func__ ""  
  # endif  
  #endif
#endif //__ANDROID__

//debug flag
#define __FFMPEG_ADAPTER_DEBUG__ 1

#ifdef __FFMPEG_ADAPTER_DEBUG__

  #ifdef __ANDROID__
    #define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, (TAG), __VA_ARGS__) 
    #define LOGI(...) __android_log_print(ANDROID_LOG_INFO, (TAG), __VA_ARGS__) 
    #define LOGW(...) __android_log_print(ANDROID_LOG_WARN, (TAG), __VA_ARGS__) 
    #define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, (TAG), __VA_ARGS__)
    #define LOGF(...) __android_log_print(ANDROID_LOG_FATAL, (TAG), __VA_ARGS__)
  #else
    #define LOGD(...) printf(__VA_ARGS__)
    #define LOGI(...) printf(__VA_ARGS__)
    #define LOGW(...) printf(__VA_ARGS__)
    #define LOGE(...) printf(__VA_ARGS__)
    #define LOGF(...) printf(__VA_ARGS__)
  #endif

#else //__FFMPEG_ADAPTER_DEBUG__

#ifdef __ANDROID__
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, (TAG), __VA_ARGS__)
#else
#define LOGE(...) printf(__VA_ARGS__)
#endif //__ANDROID__

#define LOGD(...)
#define LOGI(...)
#define LOGW(...)
#define LOGF(...)

#endif //__FFMPEG_ADAPTER_DEBUG__


typedef enum {
  ErrorBase = 100, //native error code start from 100
  ErrorGeneral,
  ErrorAllocFailed,
  ErrorParametersInvalid,
  ErrorBitmapFormat,
  ErrorNoContext,
  ErrorNoGlobalInstance,
  ErrorNoStreamFound,
  ErrorNoDecoderFound,
  ErrorNoEncoderFound,
  ErrorDuplicatedInit,
  ErrorDecodeFailed,
  ErrorBufferNotReady,
  ErrorContextExistsAlready,
  ErrorConvertError
} FFmpegAdapterNativeError;

#endif //__INCLUDE_FFMPEG_ADAPTER_COMMON_H__
