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
#include "android/opengl/OpenglEsPipeServer.h"

#include "android/base/Log.h"
#include "android/base/threads/Thread.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/synchronization/ConditionVariable.h"
#include "android/opengles.h"
#include "android/opengles-pipe.h"
#include "android/opengl/GLProcessPipe.h"
#include "android/utils/gl_cmd_net_format.h"

#include <atomic>

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <memory>

#include "asio.hpp"


// Set to 1 or 2 for debug traces
#define DEBUG 0

#if DEBUG >= 1
#define D(...) printf(__VA_ARGS__), printf("\n"), fflush(stdout)
#else
#define D(...) ((void)0)
#endif

#if DEBUG >= 2
#define DD(...) printf(__VA_ARGS__), printf("\n"), fflush(stdout)
#else
#define DD(...) ((void)0)
#endif

#if DEBUG >= 3
#define DDD(...) printf(__VA_ARGS__), printf("\n"), fflush(stdout)
#else
#define DDD(...) ((void)0)
#endif

using ChannelBuffer = emugl::RenderChannel::Buffer;
using emugl::RenderChannel;
using emugl::RenderChannelPtr;
using ChannelState = emugl::RenderChannel::State;
using IoResult = emugl::RenderChannel::IoResult;
using AsioIoService = asio::io_service;
using AsioTCP = asio::ip::tcp;
using ConditionVariable = android::base::ConditionVariable;
using Lock = android::base::Lock;

#define DEFAULT_PORT    (23432)
#define CHANNEL_BUF_CAP (1024)

namespace android {
namespace opengl {

namespace {

class EmuglPipeServerListener;
class EmuglPipeConnection;

EmuglPipeServerListener *globalPipeServer = nullptr;

class EmuglPipeConnection : android::base::Thread{
public:
    EmuglPipeConnection(AsioTCP::socket socket) : mSocket(std::move(socket)) {
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

        mIsWorking = true;

        DD("%s: create", __func__);

        // Start send thread
        start();
        
    }

    ~EmuglPipeConnection() {    
        DD("%s:", __func__);
    }

    bool isWorking() {
        return mIsWorking;
    }

    virtual intptr_t main() override {
        // Inject callback to channel
        mChannel->setEventCallback(
            [this](RenderChannel::State events) {
                this->onChannelHostEvent(events);
            });
        
        asio::async_read(
            mSocket,
            asio::buffer(&mRcvPacketHead, PACKET_HEAD_LEN),
            [this](const asio::error_code& error, size_t bytes_rcvd)
            {
                handleHeadReceiveFrom(error, bytes_rcvd);
            });
        
        DDD("%s: Send to guest thread start working", __func__);
        while (mIsWorking) {
            DDD("%s: waiting for rendering thread processing", __func__);
            mRenderResultReady.wait(&mLock);
            DDD("%s: found processed data from render thread", __func__);

            if (mIsWorking) {
                onGuestRecv();
            }
        }
        return 0;
    }

private:
     // Called to signal the guest that read/write wake events occured.
    // Note: this can be called from either the guest or host render
    // thread.
    void signalState(ChannelState state) {
        DDD("%s: Try to signal state:%d", __func__, (int)state);
        if ((state & ChannelState::CanRead) != 0) {
            mRenderResultReady.signal();
        }
    }

    // Called when an i/o event occurs on the render channel
    void onChannelHostEvent(ChannelState state) {
        DDD("%s: events %d", __func__, (int)state);
        // NOTE: This is called from the host-side render thread.
        // but closeFromHost() and signalWake() can be called from
        // any thread.
        if ((state & ChannelState::Stopped) != 0) {
            //this->closeFromHost();
            return;
        }
        signalState(state);
    }

    void closeFromHost()
    {
        mIsWorking = false;
        mChannel->stop();

        mRenderResultReady.signal(); // In case of wait
        wait();

        delete this;
    }
    
	void onGuestClose() {
        DDD("%s", __func__);
        mIsWorking = false;

        // Wait for this thread to exit
        mRenderResultReady.signal(); // In case of wait
        wait();

        mChannel->stop();

        //mSocket.close();

        DDD("%s: End receive thread", __func__);
    }


    void onGuestRecv() {
        DD("%s", __func__);

        while ((mChannel->state() & ChannelState::CanRead) != 0) {
            auto result = mChannel->tryRead(&mDataFromRender);
            DDD("%s: Trying send data to client (%d). result:%d", __func__, (int)(mDataFromRender.size()), (int)result);
            if (result != IoResult::Ok) {
                if (result == IoResult::TryAgain) {
                    continue;
                }
                fprintf(stderr, "Cannot read channel data\n");
                assert(false);
            }

            GLCmdPacketHead packetHead;
            packetHead.packet_type = DATA_PACKET;
            packetHead.packet_body_size = mDataFromRender.size();

            //uint8_t * packet = (uint8_t*)malloc(mDataFromRender.size());
            //memcpy(packet, mDataFromRender.data(), mDataFromRender.size());

            asio::error_code ec;
            size_t sentLen = asio::write(mSocket, asio::buffer(&packetHead, PACKET_HEAD_LEN), ec);
            if (sentLen != PACKET_HEAD_LEN) {
                assert(0);
                }
            
            sentLen = asio::write(mSocket, asio::buffer(mDataFromRender.data(), mDataFromRender.size()), ec);
            if (sentLen != mDataFromRender.size()) {
                assert(0);
                }

            //delete packetHead;
            //delete packet;

            DD("%s: Send data to client.(%d bytes)", __func__, (int)(PACKET_HEAD_LEN + mDataFromRender.size()));
            
            //asio::async_write(
            //    mSocket,
            //    asio::buffer(packetHead, PACKET_HEAD_LEN),
            //    [this, packetHead](const asio::error_code& ec, std::size_t bytes_transferred) {
            //        if (ec) {
            //            fprintf(stderr, "Cannot send data to client.(%d:%s)\n", ec.value(), ec.message().c_str());
            //        } else {
            //            DD("%s: Send head to client.(%d bytes)", __func__, (int)bytes_transferred);
            //        }

            //        delete packetHead;
  //              });

    //        asio::async_write(
      //          mSocket,
        //        asio::buffer(packet, mDataFromRender.size()),
          //      [this, packet](const asio::error_code& ec, std::size_t bytes_transferred) {
            //        if (ec) {
              //          fprintf(stderr, "Cannot send data to client.(%d:%s)\n", ec.value(), ec.message().c_str());
                //    } else {
                  //      DD("%s: Send data to client.(%d bytes)", __func__, (int)bytes_transferred);
                    //}

                  //  delete packet;
                //});
        }

        

        return;
    }

    int onGuestSend(const AndroidPipeBuffer* buffers,
                            int numBuffers){
        DDD("%s", __func__);

        if (!mIsWorking) {
            DDD("%s: pipe already closed!", __func__);
            return PIPE_ERROR_IO;
        }

        // Count the total bytes to send.
        int count = 0;
        for (int n = 0; n < numBuffers; ++n) {
            count += buffers[n].size;
        }

        // Copy everything into a single ChannelBuffer.
        ChannelBuffer outBuffer;
        outBuffer.resize_noinit(count);
        auto ptr = outBuffer.data();
        for (int n = 0; n < numBuffers; ++n) {
            memcpy(ptr, buffers[n].data, buffers[n].size);
            ptr += buffers[n].size;
        }

        DD("%s: sending %d bytes to host", __func__, count);
        // Send it through the channel.
        auto result = mChannel->tryWrite(std::move(outBuffer));
        if (result != IoResult::Ok) {
            DDD("%s: tryWrite() failed with %d", __func__, (int)result);
            assert(0);
            return result == IoResult::Error ? PIPE_ERROR_IO : PIPE_ERROR_AGAIN;
        }

        return count;
    }

    void onGuestWantWakeOn(int flags) {
        DD("%s:", __func__);

        // Translate |flags| into ChannelState flags.
        ChannelState wanted = ChannelState::Empty;
        if (flags & PIPE_WAKE_READ) {
            wanted |= ChannelState::CanRead;
        }
        if (flags & PIPE_WAKE_WRITE) {
            wanted |= ChannelState::CanWrite;
        }

        // Signal events that are already available now.
        ChannelState state = mChannel->state();
        ChannelState available = state & wanted;
        DD("flags, wanted, state, available:%d, %d, %d, %d", flags, (int)wanted, (int)state, (int)available);
        if (available != ChannelState::Empty) {
            DD("%s: signaling events %d", __func__, (int)available);
            signalState(available);
            wanted &= ~available;
        }

        // Ask the channel to be notified of remaining events.
        if (wanted != ChannelState::Empty) {
            DD("%s: waiting for events %d", __func__, (int)wanted);
            mChannel->setWantedEvents(wanted);
        }
    }

    void handleHeadReceiveFrom(const asio::error_code& error, size_t bytes_rcved) {
        if (!error) {
            assert(bytes_rcved == PACKET_HEAD_LEN);

            uint8_t packet_type = mRcvPacketHead.packet_type;

            DDD("%s: packet_type = %d", __func__, packet_type);

            if (packet_type == CTRL_PACKET_GUEST_CLOSE) {
                DDD("%s: try to close pipe server", __func__);
                onGuestClose();
                delete this;
                return;
            }

    		if (packet_type != DATA_PACKET) {
                printf("wrong packet_type = %d\n", packet_type);
    			assert(0);
    		}

    		mRcvPacketBodyLen = mRcvPacketHead.packet_body_size;
            assert(mRcvPacketBodyLen != 0);
            mRcvPacketData = (uint8_t *)malloc(mRcvPacketBodyLen);

    		asio::async_read(
                mSocket,
                asio::buffer(mRcvPacketData, mRcvPacketBodyLen),
                [this](const asio::error_code& error, size_t bytes_rcvd) {
                    handleBodyReceiveFrom(error, bytes_rcvd);
                });

        }else {
            printf("network error : %s\n", error.message().c_str());
            mSocket.close();
        }
        

		return;
    }

    void handleBodyReceiveFrom(const asio::error_code& error, size_t bytes_rcved) {
        if (!error) {
            DD("handleBodyReceiveFrom body:%d", (int)(bytes_rcved + PACKET_HEAD_LEN));
            assert(bytes_rcved == mRcvPacketBodyLen);

            AndroidPipeBuffer pipeBuffer = {mRcvPacketData, mRcvPacketBodyLen};
            onGuestSend(&pipeBuffer, 1);

    		int wantEvents = PIPE_WAKE_READ;
            onGuestWantWakeOn(wantEvents);

            asio::async_read(
                mSocket,
                asio::buffer(&mRcvPacketHead, PACKET_HEAD_LEN),
                [this](const asio::error_code& error, size_t bytes_rcved) {
                    handleHeadReceiveFrom(error, bytes_rcved);
                });
                }
        else {
            printf("network error : %s\n", error.message().c_str());
            mSocket.close();
        }
    }

private:
    // Set to |true| if the pipe is in working state, |false| means we're not
    // initialized or the pipe is closed.
    bool mIsWorking = false;
    
    AsioTCP::socket  mSocket;
	RenderChannelPtr mChannel;

    Lock mLock;
    ConditionVariable mRenderResultReady;

    ChannelBuffer mDataFromRender;
    size_t mDataFromRenderSize = 0;

    GLCmdPacketHead   mSndPacketHead;
    GLCmdPacketHead   mRcvPacketHead;

    uint64_t  mRcvPacketBodyLen    = 0;
    uint64_t  mRcvPacketDataCap    = 0;
    uint8_t  *mRcvPacketData       = nullptr;
};

class EmuglPipeServerListener : public android::base::Thread {
public:
    EmuglPipeServerListener() : android::base::Thread(), mIoService(), mSocket(mIoService) {
        DDD("%s", __func__);
        char *render_svr_port = getenv("render_svr_port");
        if (render_svr_port) {
            printf("Render server port: %s\n", render_svr_port);
            mEndPoint = AsioTCP::endpoint(AsioTCP::v4(), atoi(render_svr_port));
        } else {
            mEndPoint = AsioTCP::endpoint(AsioTCP::v4(), DEFAULT_PORT);
        }
        mAcceptor = new AsioTCP::acceptor(mIoService, mEndPoint);
        startAccept();
    }

    ~EmuglPipeServerListener() {
        DDD("%s", __func__);
        if (mAcceptor != nullptr) {
            delete mAcceptor;
        }
    }

    void startAccept() {
        DDD("%s: start accepting", __func__);
        mAcceptor->async_accept(
            mSocket,
            [this](std::error_code ec) {
                if (!ec) {
                    DDD("\n\n%s, new  EmuglSockPipe", __func__);
                    new EmuglPipeConnection(std::move(mSocket));
                } else {
                    fprintf(stderr, "Cannot accept client.(%d:%s)\n", ec.value(), ec.message().c_str());
                }
                startAccept();
            });
        DDD("%s: done accepting", __func__);
    }

    virtual intptr_t main() override {
        DDD("%s: io sevice start working", __func__);
        mIoService.run();
        DDD("%s: io sevice stop working", __func__);
        return 0;
    }

private:
    AsioIoService      mIoService;
    AsioTCP::endpoint  mEndPoint;
    AsioTCP::acceptor *mAcceptor;
    AsioTCP::socket    mSocket;
};

}  // namespace

void registerPipeServerService() {
    globalPipeServer = new EmuglPipeServerListener();
    globalPipeServer->start();

    registerGLProcessPipeService();
}

}  // namespace opengl
}  // namespace android

// Declared in android/opengles-pipe.h
void android_init_opengles_server_pipe() {
    android::opengl::registerPipeServerService();
}
