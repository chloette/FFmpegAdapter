FFmpegAdapter README
========

##About
`FFmpegAdapter` is a simple adapter with FFmpeg, supporting ***decode/encode frames/bitmap/buffer from/to video/audio*** easily. It includes Java part, JNI part and C part; AND **it's even possible to debug FFmpegAdapter on `Visual Studio 2013` directly.**

[FFmpeg native libraries comes from [ffmpeg4android](https://github.com/chloette/ffmpeg4android)]

##Architecture (TODO)
* Java -- FFmpegAdapter.java
* JNI -- ffmpeg_adapter_jni.h/cpp
* C/C++ -- other codes

##How to use
####win32:
(***Notice:*** win32 does not using latest windows ffmpeg libraries.)
* install Visual Studio 2013 express (or other version, probably work)
* run win32/ffmpeg_adapter/ffmpeg_adapter.sln
* DEBUG (or press F5), that's all.
* And, you'd better check `ffmpeg_adapter_win_main.cpp` first, to make sure the `test_files` folder is not empty.

####JNI
(***Notice:*** using [ffmpeg2.6](http://ffmpeg.org/))
* Makesure you know what NDK is.
* Then just `cd` to android/FFmpegAdapter/jni
* Run `ndk-build -B`
* That's all, libraries are ready (under android/FFmpegAdapter/libs/armeabi-v7a).
* BTW, ffmpeg libraries come from [ffmpeg4android](https://github.com/chloette/ffmpeg4android), DO NOT forget put `ffmpeg2.x/android` folder under `$NDK_HOME/sources/ffmpeg4android/android` before `ndk-build -B`.

####Java
* Just inherit from android/FFmpegAdapter/src/com/android/ffmpegadapter/FFmpegAdapter.java

##Contact
mailto: chloette.e@gmail.com

Have fun~ ^_^
