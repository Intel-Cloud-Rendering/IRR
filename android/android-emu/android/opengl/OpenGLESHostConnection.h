// Copyright 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#pragma once

#include "android/base/Log.h"


#include "android/base/async/Looper.h"
#include "android/base/async/ThreadLooper.h"


#include "android/opengles.h"
#include "android/opengles-pipe.h"
#include "android/emulation/AndroidPipe.h"

#include "android/utils/gl_cmd_net_format.h"
#include "android/utils/debug.h"

#include <atomic>

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <list>
#include <memory>

using emugl::RenderChannel;
using emugl::RenderChannelPtr;
using ChannelBuffer = emugl::RenderChannel::Buffer;
using IoResult = emugl::RenderChannel::IoResult;

using ChannelState = emugl::RenderChannel::State;


namespace android {
namespace opengl {

class OpenGLESHostDataHandler;
class OpenGLESHostServerConnection {
public:
    
    OpenGLESHostServerConnection(int socket, OpenGLESHostDataHandler * dataHandler);
    ~OpenGLESHostServerConnection() {
        CloseConnection();
    };

    int socketFD() {
        return mFd;
    }

    // Called when an i/o event occurs on the render channel
    void onChannelHostEvent(ChannelState state);

    void CloseConnection();
    void ActivateChannelReadNotifier();

    ssize_t receiveDataFromSocketNonBlocking(char * buf, ssize_t wantReadLen);
    bool onNetworkDataReady();

    bool onRenderChannelDataReady();

private:

    ssize_t mRecvPacketHeadLeftLen;
    ssize_t mRecvPacketBodyLeftLen;

    GLCmdPacketHead mRecvingPacketHead;
    int mSessionId;
    int mChannelId;

    int mFd;

    RenderChannelPtr mChannel;
    
    uint64_t mTotalDataSize;

    OpenGLESHostDataHandler * mDataHandler;

    ChannelBuffer mDataFromRender;
    ssize_t mSendPacketLeftLen;

};

}  // namespace emulation
}  // namespace android



