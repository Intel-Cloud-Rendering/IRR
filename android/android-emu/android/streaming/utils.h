
#ifndef STREAM_UTILS_H
#define STREAM_UTILS_H
#include "android/utils/compiler.h"

ANDROID_BEGIN_HEADER

struct IrrStreamInfo {
    /* Output-only parameters */
    int bitrate; ///< Encoder bitrate, default 1M
    int gop_size; ///< Group Of Picture size, default 50
    const char *codec; ///< Encoder codec, e.x. h264_qsv; may be null
    const char *format; ///< Mux format, e.x. flv; null as auto
    const char *url; ///< Output url.
    int low_power; ///< Enable low-power mode, default not.
    const char *res; ///< Encoding resolution.
    const char *framerate; ///< Encoding framerate
    const char *exp_vid_param; ///< Extra encoding/muxer parameters passed to libtrans/FFmpeg
};

/**
 * @param w           screen width
 * @param h           screen height
 * @param framerate   screen refresh frequency
 * @desc              Register a screen capture and encoding into a stream
 */
void register_stream_publishment(int w, int h, float framerate);

/**
 * @param stream_info
 * @return 0 on success
 */
int irr_stream_start(struct IrrStreamInfo *stream_info);

void irr_stream_stop();

/*
 * @Desc Stop and release all streaming-related resources.
 *       It's maintained by an auto-ptr in fact. So you can feel
 *       free for not calling this function.
 */
void unregister_stream_publishment();

/*
 * @Desc Update the frame buffer.
 * @Note w/h is not really matter here. So if w or h is changed,
 *       remember to call @unregister_stream_publishment() first.
 */
int fresh_screen(int w, int h, const void *pixels);

/*
 * @Desc get a buffer from streamer.
 *       This is designed to reduce memory-copy times.
 */
void *irr_get_buffer(int size);

ANDROID_END_HEADER
#endif /* STREAM_UTILS_H */
