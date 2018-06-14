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

#include <memory>
#include "stream.h"

using IrrStreamInfo = IrrStreamer::IrrStreamInfo;

static std::unique_ptr<IrrStreamer> pStreamer = nullptr;

void register_stream_publishment(int w, int h, float framerate) {
    pStreamer.reset(new IrrStreamer(w, h, framerate));
}

int fresh_screen(int w, int h, const void *pixels) {
    if (!pStreamer.get())
        return -EINVAL;

    return pStreamer->write(pixels, w * h * 4);
}

void unregister_stream_publishment() {
    if (!pStreamer.get())
        return;

    pStreamer = nullptr;
}

void *irr_get_buffer(int size) {
    static std::unique_ptr<uint8_t> pBuffer = nullptr;

    if (!pStreamer.get()) {
        if (!pBuffer.get()) {
            pBuffer.reset(new uint8_t[size]);
        }
        return nullptr;
    }

    return pStreamer->getBuffer();
}

int irr_stream_start(IrrStreamInfo *stream_info) {
    if (!pStreamer.get())
        return -EINVAL;

    return pStreamer->start(stream_info);
}

void irr_stream_stop() {
    if (pStreamer.get())
        pStreamer->stop();
}

int irr_stream_force_keyframe(int force_key_frame) {
    if (!pStreamer.get())
        return -EINVAL;

    return pStreamer->force_key_frame(force_key_frame);
}
