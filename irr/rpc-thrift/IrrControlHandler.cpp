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
#include "Dump.h"
#include "stream.h"

using namespace ::IntelCloudRendering;

IrrControlHandler::IrrControlHandler() {
    // Your initialization goes here
}

IrrControlHandler::~IrrControlHandler() {
}

void IrrControlHandler::ping() {
    // Your implementation goes here
    D("ping\n");
}


int32_t IrrControlHandler::startDump(const DumpInfo& info) {
    D("startDump\n");

    return irr::start_dump_cmd(info);
}

int32_t IrrControlHandler::stopDump() {
    D("stopDump\n");

    return irr::stop_dump_cmd();
}

int32_t IrrControlHandler::restartDump(const DumpInfo& info) {
    D("restartDump\n");

    irr::stop_dump_cmd();
    return irr::start_dump_cmd(info);
}

bool IrrControlHandler::readDumpStatus() {
    D("readDumpStatus\n");

    return irr::is_dump();
}

int32_t IrrControlHandler::startStream(const StreamInfo &info) {
    IrrStreamer::IrrStreamInfo stream_info = {0};

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

int32_t IrrControlHandler::forceKeyFrame(int32_t force_key_frame) {
    printf("force key framw now\n");
    return irr_stream_force_keyframe(force_key_frame);
}
