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

#include "android/base/async/ScopedSocketWatch.h"

#include "android/opengles.h"
#include "android/opengles-pipe.h"
#include "android/emulation/AndroidPipe.h"
#include "android/emulation/VmLock.h"

#include "android/utils/gl_cmd_net_format.h"
#include "android/utils/debug.h"
#include "android/opengl/OpenGLESHostListener.h"

#include <atomic>

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <list>
#include <memory>

using emugl::RenderChannel;
using emugl::RenderChannelPtr;
using ChannelBuffer = emugl::RenderChannel::Buffer;
using IoResult = emugl::RenderChannel::IoResult;

using ScopedSocket = ::android::base::ScopedSocket;
using ChannelState = emugl::RenderChannel::State;
using FdWatch = android::base::Looper::FdWatch;


namespace android {
namespace opengl {

//

class OpenGLESHostServerConnection {
public:
    
    OpenGLESHostServerConnection(int socket, OpenGLESHostDataHandler * dataHandlerPtr) {
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

        mChannel = renderer->createRenderChannel();
        if (!mChannel) {
            fprintf(stderr, "Failed to create an OpenGLES pipe channel!");
            return;
        }

        mChannel->setEventCallback(
            [this](RenderChannel::State events) {
                this->onChannelHostEvent(events);
            });

        mDataHandlerPtr = dataHandlerPtr;
        
        android::base::socketSetNonBlocking(socket);
        // socketSetNoDelay() reduces the latency of sending data, at the cost
        // of creating more TCP packets on the connection. It's useful when
        // doing lots of small send() calls, like the ADB protocol requires.
        // And since this is on localhost, the packet increase should not be
        // noticeable.
        android::base::socketSetNoDelay(socket);

        mSocket.reset(android::base::ThreadLooper::get()->createFdWatch(
                socket,
                [](void* opaque, int fd, unsigned events) {
                    ((OpenGLESHostServerConnection*)opaque)->onHostSocketEvent(events);
                },
                this));

        if (mSocket.get()) {
            mSocket->wantRead();
        }
        
        DDD("%s: create", __func__);
    };

    ~OpenGLESHostServerConnection() {};

    void onHostSocketEvent(unsigned events) {
        if ((events & FdWatch::kEventRead) != 0) {
            //mSocket->dontWantRead();
            mDataHandlerPtr->PushBack(std::bind(&OpenGLESHostServerConnection::onNetworkDataReady, this));
            //mDataHandlerPtr->NotifyDataReady();
        }

        //if ((events & FdWatch::kEventWrite) != 0) {
        //    printf("ready to write socket\n");
        //    mSocket->dontWantWrite();
        //    mDataHandlerPtr->PushBack(std::bind(&OpenGLESHostConnection::onDataWriteReady, this));
  //          mDataHandlerPtr->NotifyDataReady();
        //}
    }

    // Called when an i/o event occurs on the render channel
    void onChannelHostEvent(ChannelState state) {
        DDD("%s: events %d\n", __func__, (int)state);
        // NOTE: This is called from the host-side render thread.
        // but closeFromHost() and signalWake() can be called from
        // any thread.
        if ((state & ChannelState::Stopped) != 0) {
            //this->closeFromHost();
            return;
        }

        if ((state & ChannelState::CanRead) != 0) {
            mDataHandlerPtr->PushBack(std::bind(&OpenGLESHostServerConnection::onRenderChannelDataReady, this));
        }
        //signalState(state);
    }

    void CloseConnection() {

        mChannel->stop();

        mSocket.reset();
    }
    
    void ActivateChannelReadNotifier() {

            ChannelState flag = ChannelState::CanRead;
    
            // Signal events that are already available now.
            ChannelState state = mChannel->state();
            ChannelState available = state & flag;

            if (available != ChannelState::Empty) {
                //mSocket->wantWrite();
                mDataHandlerPtr->PushBack(std::bind(&OpenGLESHostServerConnection::onRenderChannelDataReady, this));
                return;
            }

            mChannel->setWantedEvents(flag);
        }

    void onNetworkDataReady() {
        DD("%s : \n", __func__);\
        if (!mSocket) {
            //assert(0);
            return;
            }

        if (mRecvPacketBodyLeftLen == 0) {
            ssize_t headLen;
            {
                ScopedVmUnlock unlockBql;
                headLen = android::base::socketRecv(mSocket->fd(),
                                                &mRecvingPacketHead + PACKET_HEAD_LEN - mRecvPacketHeadLeftLen, mRecvPacketHeadLeftLen);
            }

            if (headLen < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    mSocket->wantRead();
                    return;
                    }
                else {
                    mSocket->dontWantRead();
                    mSocket.reset();
                    assert(0);
                    return;
                }
            }

            mRecvPacketHeadLeftLen -= headLen;

            if (mRecvPacketHeadLeftLen > 0) {
                mSocket->wantRead();
                return;
                }
            uint8_t packet_type = mRecvingPacketHead.packet_type;
            //mSessionId = mRecvingPacketHead.session_id;

            DD("%s: packet_type = %d\n", __func__, packet_type);

            if (packet_type == CTRL_PACKET_GUEST_CLOSE) {
                printf("%s: try to close pipe server\n", __func__);
                CloseConnection();
                return;
            }

    		if (packet_type != DATA_PACKET) {
                printf("wrong packet_type = %d\n", packet_type);
                mSocket->dontWantRead();
    			assert(0);
    		}

            mRecvPacketBodyLeftLen = mRecvingPacketHead.packet_body_size;
            assert(mRecvPacketBodyLeftLen != 0);

            mSocket->wantRead();

        }
        else {
            ChannelBuffer outBuffer;
            outBuffer.resize_noinit(mRecvPacketBodyLeftLen);

            //ssize_t recvLen = outBuffer.capacity();
            //if (mRecvPacketBodyLeftLen < (ssize_t)outBuffer.capacity())
            //    recvLen = mRecvPacketBodyLeftLen;
            
            auto packet_data = outBuffer.data();
        
            ssize_t bodyLen;
            {
                ScopedVmUnlock unlockBql;
                bodyLen = android::base::socketRecv(mSocket->fd(),
                                                packet_data,
                                                outBuffer.size());
            }

            if (bodyLen < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    mSocket->wantRead();
                    return;
                    }
                else {
                    mSocket->dontWantRead();
                    mSocket.reset();
                    assert(0);
                    return;
                }
            }

            mRecvPacketBodyLeftLen -= bodyLen;

            if (mRecvPacketBodyLeftLen > 0)
                outBuffer.resize_noinit(bodyLen);
        
            DD("read %d bytes data from socket (%d), write to render channel\n", (int)bodyLen, mSessionId);
            auto result = mChannel->tryWrite(std::move(outBuffer));
            if (result != IoResult::Ok) {
                DDD("%s: tryWrite() failed with %d", __func__, (int)result);
                mSocket->dontWantRead();
                assert(0);
                return;
            }

            if (mRecvPacketBodyLeftLen == 0) {
                mRecvPacketHeadLeftLen = PACKET_HEAD_LEN;
                ActivateChannelReadNotifier();
             }

            mSocket->wantRead();
        }
    }

    void onRenderChannelDataReady() {
        //DD("%s : ", __func__);
        if (!mSocket)
            return;
        
        ChannelBuffer DataFromRender;
        while ((mChannel->state() & ChannelState::CanRead) != 0) {
            auto result = mChannel->tryRead(&DataFromRender);

            if (result != IoResult::Ok) {
                if (result == IoResult::TryAgain) {
                    break;
                }
                fprintf(stderr, "Cannot read channel data\n");
                assert(false);
            }
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
            ssize_t sentLen = 0;
            while (1) {
                ssize_t retBody;
                {
                    ScopedVmUnlock unlockBql;
                    retBody = android::base::socketSend(mSocket->fd(),
                                                    DataFromRender.data() + sentLen, DataFromRender.size() - sentLen);
                }
                
                if (retBody < 0) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        continue;
                        }
                    else {
                        mSocket.reset();
                        assert(0);
                        return;
                    }
                }

                sentLen += retBody;

                if (sentLen == (ssize_t)DataFromRender.size())
                    break;

            }

            DD("write to client(%d) %d bytes from render channel\n", mSessionId, (int)DataFromRender.size());

        }

        mSocket->wantRead();
    }

private:

    ssize_t mRecvPacketHeadLeftLen;
    ssize_t mRecvPacketBodyLeftLen;

    GLCmdPacketHead mRecvingPacketHead;
    int mSessionId;
    android::base::ScopedSocketWatch mSocket; 
    RenderChannelPtr mChannel;

    OpenGLESHostDataHandler* mDataHandlerPtr;

    
};

}  // namespace emulation
}  // namespace android



