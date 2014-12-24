package com.android.ffmpegadapter;

import android.graphics.Bitmap;
import android.util.Log;

public class FFmpegAdapter {
    public static final String TAG = FFmpegAdapter.class.getSimpleName();

    static {
        try {
            System.loadLibrary("avutil-54");
            System.loadLibrary("swscale-3");
            System.loadLibrary("swresample-1");
            System.loadLibrary("avcodec-56");
            System.loadLibrary("avformat-56");
            System.loadLibrary("ffmpegadapter");
        } catch (UnsatisfiedLinkError e) {
            e.printStackTrace();
            Log.e(TAG, "PLEASE MAKE SURE ALL LIBRARIES WERE PLACED!");
            throw e;
        }
    }


    protected int _nativeContext = 0;

    public static final int ErrorDuplicatedStart = 1;
    public static final int ErrorNotStartedYet = 2;
    public static final int ErrorWidthHeightInvalid = 3;
    public static final int ErrorFPSInvalid = 4;
    public static final int ErrorQualityInvalid = 5;
    public static final int ErrorFileInvalid = 6;
    public static final int ErrorStartTimeInvalid = 7;
    public static final int ErrorDurationInvalid = 8;
    public static final int ErrorCritical = 9;


    protected native int nativeInit();

    protected native void nativeRelease(int context);

    protected native int nativeSetAudioStartFrom(int context, long from);

    protected native int nativeSetEncodeDuration(int context, long duration);

    protected native int nativeEncodeTo(int context, String path);

    protected native int nativeDecodeFrom(int context, String fileName);

    protected native long nativeEncodeGetDurationV(int context);

    protected native long nativeEncodeGetDurationA(int context);

    protected native int nativeEncodeSetResolution(int context, int width,
                                                   int height);

    protected native int nativeEncodeSetFps(int context, double fps);

    protected native int nativeEncodeSetVideoQuality(int context, int video_quality);

    protected native long nativeDecodeGetDurationV(int context);

    protected native long nativeDecodeGetDurationA(int context);

    protected native int nativeDecodeGetAudioFormat(int context);

    protected native int nativeDecodeGetAudioSamplerate(int context);

    protected native int nativeDecodeGetAudioChannels(int context);

    protected native int nativeDecodeGetAudioChannelLayout(int context);

    protected native int nativeDecodeGetAudioFrameSize(int context);

    protected native int nativeDecodeGetAudioBufferSize(int context);

    protected native int nativeDecodeSetAudioFormat(int context, int format);

    protected native int nativeDecodeSetAudioSamplerate(int context, int samplerate);

    protected native int nativeDecodeSetAudioChannels(int context, int channels);

    protected native int nativeDecodeSetAudioFrameSize(int context, int framesize);

    protected native int nativeDecodeEoF(int context);

    protected native int nativeDecodeSeekTo(int context, long timestamp);

    protected native int nativeDecodeFrameV(int context, long timestamp,
                                            byte[] data, int format, int out_width, int out_height, int rotate_angle);

    protected native int nativeDecodeFrameA(int context, long timestamp,
                                            byte[] data, int format, int ratio);

    protected native long nativeDecodeActualTimestampV(int context);

    protected native long nativeDecodeActualTimestampA(int context);

    protected native int nativeEncodeFrameV(int context, long timestamp,
                                            Bitmap bitmap, boolean with_audio);

    protected native int nativeEncodeBufferV(int context, long timestamp,
                                             byte[] data, int format, int in_width, int in_height, int rorate_angle);

    protected native int nativeEncodeBufferA(int context, long timestamp,
                                             byte[] data, int format, int size);

    protected native int nativeAddCompressedFrameV(int context, long timestamp,
                                                   byte[] data, int offset, int size, int flag);

    protected native int nativeAddExtraDataV(int context, byte[] data, int offset,
                                             int size);

    protected native int nativeAddCompressedFrameA(int context, long timestamp,
                                                   byte[] data, int offset, int size, int flag);

    protected native int nativeAddExtraDataA(int context, byte[] data, int offset,
                                             int size);

    protected native int nativeRgbaBitmapToRgbaBuffer(int context, Bitmap bitmap,
                                                      byte[] rgbadata);

    protected native int nativeRgbaBitmapToYuv(int context, Bitmap bitmap,
                                               byte[] yuvdata, int flag);

    protected native int nativeRgbaToYuv(int context, byte[] rgba, byte[] yuv,
                                         int width, int height, int flag);

    protected native int nativeGetLastError(int context);

    public static native int nativeConcatFiles(String[] paths, String outputPath);
}
