// Copyright 2018 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#ifndef STREAM_UTILS_H
#define STREAM_UTILS_H

#include "IrrStreamer.h"

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
int irr_stream_start(IrrStreamer::IrrStreamInfo *stream_info);

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

/*
 * @Desc force key frame
 */
int irr_stream_force_keyframe(int force_key_frame);

#endif /* STREAM_UTILS_H */
