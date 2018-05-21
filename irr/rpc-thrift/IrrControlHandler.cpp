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

#include "IrrControlHandler.h"
#include "android/streaming/utils.h"

using namespace ::IntelCloudRendering;

IrrControlHandler::IrrControlHandler() {
    // Your initialization goes here
}

IrrControlHandler::~IrrControlHandler() {
}

void IrrControlHandler::ping() {
    // Your implementation goes here
    printf("ping\n");
}

int32_t IrrControlHandler::startStream(const StreamInfo &info) {
    IrrStreamInfo stream_info = {0};

    if (info.__isset.framerate)
        stream_info.framerate    = info.framerate.c_str();
    if (info.__isset.exp_vid_param)
        stream_info.exp_vid_param = info.exp_vid_param.c_str();
    if (info.__isset.vcodec)
        stream_info.codec        = info.vcodec.c_str();
    if (info.__isset.format)
        stream_info.format       = info.format.c_str();
    if (info.__isset.resolution)
        stream_info.res          = info.resolution.c_str();

    stream_info.url = info.url.c_str();

    return irr_stream_start(&stream_info);
}

void IrrControlHandler::stopStream() {
    irr_stream_stop();
}

int32_t IrrControlHandler::restartStream(const StreamInfo &info) {
    stopStream();

    return startStream(info);
}
