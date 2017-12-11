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

#include "android/base/Log.h"

#include "android/base/sockets/SocketUtils.h"

#include "android/opengles.h"
#include "android/opengles-pipe.h"
#include "android/emulation/AndroidPipe.h"
#include "android/emulation/VmLock.h"


#include "android/utils/gl_cmd_net_format.h"
#include "android/utils/debug.h"
#include "android/opengl/OpenGLESHostConnection.h"
#include "android/opengl/OpenGLESHostDataHandler.h"


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
    
OpenGLESHostServerConnection::OpenGLESHostServerConnection(int socket, OpenGLESHostDataHandler * dataHandler) :
        mFd(socket),
        mTotalDataSize(0),
        mDataHandler(dataHandler),
        mSendPacketLeftLen(0) {


    mRecvPacketHeadLeftLen = PACKET_HEAD_LEN;
    mRecvPacketBodyLeftLen = 0;
    
    auto renderer = android_getOpenglesRenderer();
    if (!renderer) {
        // This should never happen, unless there is a bug in the
        // emulator's initialization, or the system image.
        D("Trying to open the OpenGLES pipe without GPU emulation!");
        return;
    }
    DDD("%s: create", __func__);

    int peerPort = android::base::socketGetPeerPort(socket);
    mChannel = renderer->createRenderChannel(peerPort);
    if (!mChannel) {
        fprintf(stderr, "Failed to create an OpenGLES pipe channel!");
        return;
    }

    mChannel->setEventCallback(
        [this](RenderChannel::State events) {
            this->onChannelHostEvent(events);
        });
    
    android::base::socketSetNonBlocking(socket);
    // socketSetNoDelay() reduces the latency of sending data, at the cost
    // of creating more TCP packets on the connection. It's useful when
    // doing lots of small send() calls, like the ADB protocol requires.
    // And since this is on localhost, the packet increase should not be
    // noticeable.
    android::base::socketSetNoDelay(socket);

    DDD("%s: create", __func__);
};

// Called when an i/o event occurs on the render channel
void OpenGLESHostServerConnection::onChannelHostEvent(ChannelState state) {
    //printf("%s: events %d\n", __func__, (int)state);
    // NOTE: This is called from the host-side render thread.
    // but closeFromHost() and signalWake() can be called from
    // any thread.
    if ((state & ChannelState::Stopped) != 0) {
        //this->closeFromHost();
        printf("%s: event stop %d\n", __func__, (int)state);
        return;
    }

    if ((state & ChannelState::CanRead) != 0) {
        mDataHandler->modConnection(mFd, false);
    }
}

void OpenGLESHostServerConnection::ActivateChannelReadNotifier() {

    ChannelState flag = ChannelState::CanRead;

    // Signal events that are already available now.
    ChannelState state = mChannel->state();
    ChannelState available = state & flag;

    if (available != ChannelState::Empty) {
        //onRenderChannelDataReady();
        mDataHandler->modConnection(mFd, false);
        //return;
    }

    mChannel->setWantedEvents(flag);
}

ssize_t OpenGLESHostServerConnection::receiveDataFromSocketNonBlocking(char * buf, ssize_t wantReadLen) {
    ssize_t readLen;
    {
        ScopedVmUnlock unlockBql;
        readLen = android::base::socketRecv(
            mFd,
            buf,
            wantReadLen);
    }

    if (readLen < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;
        } else {
            //mSocket.reset();
            assert(0);
            return -1;
        }
    }

    return readLen;
}

void OpenGLESHostServerConnection::CloseConnection() {
    android::base::socketClose(mFd);
    mFd = -1;
    mChannel->stop();
}

bool OpenGLESHostServerConnection::onNetworkDataReady() {
    //printf("%s : \n", __func__);
    if (mFd <= 0) {
        return false;
    }

    while (1) {
        if (mRecvPacketBodyLeftLen == 0) {

            int headLen = receiveDataFromSocketNonBlocking(
                (char *)(&mRecvingPacketHead + PACKET_HEAD_LEN - mRecvPacketHeadLeftLen),
                mRecvPacketHeadLeftLen);

            if (headLen == -1) {
                printf("socket %d closed\n", mSessionId);
                return false;
            }

            mRecvPacketHeadLeftLen -= headLen;

            if (mRecvPacketHeadLeftLen > 0) {
                // no data in socket buf anymore, exit the loop
                break;
            }

            uint8_t packet_type = mRecvingPacketHead.packet_type;
            //mSessionId = mRecvingPacketHead.session_id;

            DD("%s: packet_type = %d\n", __func__, packet_type);

            if (packet_type == CTRL_PACKET_GUEST_CLOSE) {
                printf("%s: try to close pipe server\n", __func__);
                CloseConnection();
                return false;
            }

            if (packet_type != DATA_PACKET) {
                printf("wrong packet_type = %d\n", packet_type);
                assert(0);
                return false;
            }

            mRecvPacketBodyLeftLen = mRecvingPacketHead.packet_body_size;
            assert(mRecvPacketBodyLeftLen != 0);
        } else {
            ChannelBuffer outBuffer;
            outBuffer.resize_noinit(mRecvPacketBodyLeftLen);

            auto packet_data = outBuffer.data();

            int bodyLen = receiveDataFromSocketNonBlocking(
                packet_data,
                outBuffer.size());

            if (bodyLen == -1) {
                assert(0);
                return false;
            }

            mRecvPacketBodyLeftLen -= bodyLen;

            if (mRecvPacketBodyLeftLen > 0) {
                // no data in socket buf anymore, exit the loop
                assert(bodyLen <= (ssize_t)(outBuffer.size()));
                outBuffer.resize_noinit(bodyLen);
            }
            mTotalDataSize += (int)bodyLen;
            //printf("read %d bytes data from socket (%d), write to render channel %ld\n",
            //    (int)bodyLen, mSessionId, mTotalDataSize);
            auto result = mChannel->tryWrite(std::move(outBuffer));
            if (result != IoResult::Ok) {
                DDD("%s: tryWrite() failed with %d", __func__, (int)result);
                assert(0);
                return false;
            }

            if (mRecvPacketBodyLeftLen == 0) {
                mRecvPacketHeadLeftLen = PACKET_HEAD_LEN;
                ActivateChannelReadNotifier();
            } else 
                break;
        }
        
    }

    return true;
    
}

bool OpenGLESHostServerConnection::onRenderChannelDataReady() {
    //printf("%s : \n", __func__);
    if (mFd <= 0)
        return false;

    while (1) {
        if (mSendPacketLeftLen == 0) {
            if ((mChannel->state() & ChannelState::CanRead) != 0) {
                auto result = mChannel->tryRead(&mDataFromRender);

                if (result != IoResult::Ok) {
                    if (result == IoResult::TryAgain) {
                        printf("%s TryAgain: \n", __func__);
                        break;
                    }
                    fprintf(stderr, "Cannot read channel data\n");
                    assert(0);
                    return false;
                }

                mSendPacketLeftLen = mDataFromRender.size();
            } else {
                mDataHandler->modConnection(mFd, true);
                break;
            }
        } else {
            ssize_t offset = mDataFromRender.size() - mSendPacketLeftLen;

            /* a packet head will be sent if needed.

                        //GLCmdPacketHead packetHead;
                        //packetHead.packet_type = DATA_PACKET;
                        //packetHead.packet_body_size = DataFromRender.size();

                        //bool retHead;
                        //{
                        //    ScopedVmUnlock unlockBql;
                        //    retHead = android::base::socketSendAll(mSocket->fd(),
                        //                                    &packetHead, PACKET_HEAD_LEN);
                        //}

                        //if (!retHead) {
                        //    mSocket.reset();
                        //    assert(0);
                        //    return;
                        //}
                */

            ssize_t ret;
            {
                ScopedVmUnlock unlockBql;
                ret = android::base::socketSend(mFd,
                                                mDataFromRender.data() + offset,
                                                mSendPacketLeftLen);
            }
                
            if (ret < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    break;
                } else {
                    //mSocket.reset();
                    assert(0);
                    return false;
                }
            }

            mSendPacketLeftLen -= ret;

            if (mSendPacketLeftLen == 0) {
                //printf("write to client(%d) %d bytes from render channel\n", 
                //    mSessionId, (int)mDataFromRender.size());
            }
        }
    }

    return true;
}

}
}




