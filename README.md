FFmpegAdapter README
========

##About
`FFmpegAdapter` is an adapter of FFmpeg, it includes Java part, JNI part and C part; AND **it's possible to debug FFmpegAdapter on `Visual Studio 2013` directly.**

##Architecture (TODO)
* Java -- FFmpegAdapter.java
* JNI -- ffmpeg_adapter_jni.h/cpp
* C/C++ -- other codes

##How to use
####win32:
* install Visual Studio 2013 express (or other version, probably work)
* run win32/ffmpeg_adapter/ffmpeg_adapter.sln
* DEBUG (or press F5), that's all.
* And, you'd better check `ffmpeg_adapter_win_main.cpp` first, to make sure the `test_files` folder is not empty.

####JNI
* Makesure you know what NDK is.
* Then just `cd` to android/FFmpegAdapter/jni
* Run `ndk-build -B`
* That's all, the libraries are ready now (under android/FFmpegAdapter/libs/armeabi-v7a).

####Java
* Just inherit from android/FFmpegAdapter/src/com/android/ffmpegadapter/FFmpegAdapter.java

##Contact
mailto: chloette.e@gmail.com

Have fun~ ^_^
