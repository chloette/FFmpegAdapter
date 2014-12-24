# Copyright (C) 2009 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CFLAGS 		+= -D__STDC_CONSTANT_MACROS #-D__cplusplus

LOCAL_MODULE    :=  ffmpegadapter

LOCAL_SRC_FILES :=  ffmpeg_adapter_jni.cpp \
                    ffmpeg_adapter_encoder.cpp \
                    ffmpeg_adapter_decoder.cpp \
                    ffmpeg_adapter_util.cpp \
                    ffmpeg_adapter_transform.c \
                    ffmpeg_adapter_rotate.c \
                    ffmpeg_adapter_converter.cpp
										
LOCAL_SHARED_LIBRARIES := libavformat libavcodec libswscale libswresample libavutil 

LOCAL_LDLIBS    := -llog -lz -lm -ljnigraphics

LOCAL_C_INCLUDES := $(LOCAL_PATH) \
                    ./include/ \
                    ./include/libavcodec \
                    ./include/libavdevice \
                    ./include/libavfilter \
                    ./include/libavformat \
                    ./include/libavutil \
                    ./include/libpostproc \
                    ./include/libswresample \
                    ./include/libswscale

include $(BUILD_SHARED_LIBRARY)
$(call import-module, ffmpeg4android/ffmpeg-2.4.2/android/arm)