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
#define DEBUG 3

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

class EmuglPipeServerServer;
class EmuglSockPipe;
class EmuglPipeServer;

EmuglPipeServerServer *globalPipeServer = nullptr;

class EmuglPipeServer : public AndroidPipe, public android::base::Thread {
public:
    /////////////////////////////////////////////////////////////////////////
    // Constructor, check that |mIsWorking| is true after this call to verify
    // that everything went well.
    EmuglPipeServer(void* hwPipe, Service* service,
                    const emugl::RendererPtr& renderer,
                    AsioTCP::socket &sock)
        : AndroidPipe(hwPipe, service), android::base::Thread(), mLock(), mSock(sock) {
        DDD("%s: create", __func__);
        mChannel = renderer->createRenderChannel();
        if (!mChannel) {
            fprintf(stderr, "Failed to create an OpenGLES pipe channel!");
            return;
        }

        mIsWorking = true;
        // Start send thread
        start();
        // Inject callback to channel
        mChannel->setEventCallback(
            [this](RenderChannel::State events) {
                this->onChannelHostEvent(events);
            });
    }
    
    ~EmuglPipeServer() {
        DDD("%s:", __func__);
    }

    bool isWorking() {
        return mIsWorking;
    }

    virtual intptr_t main() override {
        DD("%s: Send to guest thread start working", __func__);
        while (mIsWorking) {
            DD("%s: Start wating to send data to client", __func__);
            mReplyClient.wait(&mLock);
            DD("%s: Done wating to send data to client", __func__);
            
            if (mIsWorking) {
                onGuestRecv(nullptr, 0);
            }
        }
        return 0;
    }

    //////////////////////////////////////////////////////////////////////////
    // Overriden AndroidPipe methods

    virtual void onGuestClose() override {
        D("%s", __func__);
        mIsWorking = false;
        mChannel->stop();

        // Wait for this thread to exit
        mReplyClient.signal(); // In case of wait
        wait();

        DD("%s: End receive thread", __func__);
        delete this;
    }

    virtual unsigned onGuestPoll() override {
        DD("%s", __func__);

        unsigned ret = 0;
        if (mDataForReadingLeft > 0) {
            ret |= PIPE_POLL_IN;
        }
        ChannelState state = mChannel->state();
        if ((state & ChannelState::CanRead) != 0) {
            ret |= PIPE_POLL_IN;
        }
        if ((state & ChannelState::CanWrite) != 0) {
            ret |= PIPE_POLL_OUT;
        }
        if ((state & ChannelState::Stopped) != 0) {
            ret |= PIPE_POLL_HUP;
        }
        DD("%s: returning %d", __func__, ret);
        return ret;
    }

    virtual int onGuestRecv(AndroidPipeBuffer* buffers, int numBuffers) override {
        DD("%s", __func__);

        while ((mChannel->state() & ChannelState::CanRead) != 0) {
            auto result = mChannel->tryRead(&mDataForReading);
            DD("%s: Trying send data to client (%d). result:%d", __func__, (int)(mDataForReading.size()), (int)result);
            if (result != IoResult::Ok) {
                if (result == IoResult::TryAgain) {
                    continue;
                }
                fprintf(stderr, "Cannot read channel data\n");
                assert(false);
            }

            if (mSndBufLen == 0) {
                mSndBufLen = 2 * (PACKET_HEAD_LEN + mDataForReading.size());
                mSndBuf = (uint8_t *)malloc(mSndBufLen);
            } else {
                if (mSndBufLen < (PACKET_HEAD_LEN + mDataForReading.size())) {
                    mSndBufLen = 2 * (PACKET_HEAD_LEN + mDataForReading.size());
                    mSndBuf = (uint8_t *)realloc(mSndBuf, mSndBufLen);
                }
            }

            int paketSize = format_gl_data_command(
                mDataForReading.size(),
                (uint8_t *)(mDataForReading.data()),
                mSndBufLen,
                mSndBuf);

            asio::async_write(
                mSock,
                asio::buffer(mSndBuf, paketSize),
                [this](const asio::error_code& ec, std::size_t bytes_transferred) {
                    if (ec) {
                        fprintf(stderr, "Cannot send data to client.(%d:%s)\n", ec.value(), ec.message().c_str());
                    } else {
                        DD("%s: Send data to client.(%d bytes)", __func__, (int)bytes_transferred);
                    }
                });
        }

        return 0;
    }

    virtual int onGuestSend(const AndroidPipeBuffer* buffers,
                            int numBuffers) override {
        DD("%s", __func__);

        if (!mIsWorking) {
            DD("%s: pipe already closed!", __func__);
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

        D("%s: sending %d bytes to host", __func__, count);
        // Send it through the channel.
        auto result = mChannel->tryWrite(std::move(outBuffer));
        if (result != IoResult::Ok) {
            D("%s: tryWrite() failed with %d", __func__, (int)result);
            return result == IoResult::Error ? PIPE_ERROR_IO : PIPE_ERROR_AGAIN;
        }

        return count;
    }

    virtual void onGuestWantWakeOn(int flags) override {
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

private:
    // Called to signal the guest that read/write wake events occured.
    // Note: this can be called from either the guest or host render
    // thread.
    void signalState(ChannelState state) {
        DD("%s: Try to signal state:%d", __func__, (int)state);
        if ((state & ChannelState::CanRead) != 0) {
            mReplyClient.signal();
        }
    }

    // Called when an i/o event occurs on the render channel
    void onChannelHostEvent(ChannelState state) {
        D("%s: events %d", __func__, (int)state);
        // NOTE: This is called from the host-side render thread.
        // but closeFromHost() and signalWake() can be called from
        // any thread.
        if ((state & ChannelState::Stopped) != 0) {
            this->closeFromHost();
            return;
        }
        signalState(state);
    }

    // A RenderChannel pointer used for communication.
    RenderChannelPtr mChannel;

    // Set to |true| if the pipe is in working state, |false| means we're not
    // initialized or the pipe is closed.
    bool mIsWorking = false;

    // These two variables serve as a reading buffer for the guest.
    // Each time we get a read request, first we extract a single chunk from
    // the |mChannel| into here, and then copy its content into the
    // guest-supplied memory.
    // If guest didn't have enough room for the whole buffer, we track the
    // number of remaining bytes in |mDataForReadingLeft| for the next read().
    ChannelBuffer mDataForReading;
    size_t mDataForReadingLeft = 0;

    Lock mLock;
    ConditionVariable mReplyClient;

    AsioTCP::socket &mSock;
    uint8_t         *mSndBuf    = nullptr;
    uint8_t          mSndBufLen = 0;

    DISALLOW_COPY_ASSIGN_AND_MOVE(EmuglPipeServer);
};

class EmuglSockPipe {
public:
    EmuglSockPipe(AsioTCP::socket sock) : mSock(std::move(sock)) {
        DD("%s: create", __func__);
        mEmuglPipeServer = createEmuglPipeServer(nullptr, mSock);
        assert(mEmuglPipeServer != nullptr);

        asio::async_read(
            mSock,
            asio::buffer(mRcvPacketHead, PACKET_HEAD_LEN),
            [this](const asio::error_code& error, size_t bytes_rcvd)
            {
                handleHeadReceiveFrom(error, bytes_rcvd);
            });
    }

    ~EmuglSockPipe() {
        DD("%s:", __func__);
    }

private:
    EmuglPipeServer* createEmuglPipeServer(void* mHwPipe, AsioTCP::socket &sock) {
        DD("%s: create", __func__);
        auto renderer = android_getOpenglesRenderer();
        if (!renderer) {
            // This should never happen, unless there is a bug in the
            // emulator's initialization, or the system image.
            D("Trying to open the OpenGLES pipe without GPU emulation!");
            return nullptr;
        }

        EmuglPipeServer* pipe = new EmuglPipeServer(mHwPipe, nullptr, renderer, sock);
        if (!(pipe->isWorking())) {
            delete pipe;
            pipe = nullptr;
        }
        return pipe;
    }

    void handleSetWantEventReceiveFrom(const asio::error_code& error, size_t bytes_rcved) {
        DD("%s: error:%d, bytes_rcved:%d", __func__, error.value(), (int)bytes_rcved);
        if (!error) {
            assert(bytes_rcved == sizeof(int));
            int wantEvents = *((int *)mRcvPacketData);
            DDD("Wanted events:%d", wantEvents);
            mEmuglPipeServer->onGuestWantWakeOn(wantEvents);
            asio::async_read(
                mSock,
                asio::buffer(mRcvPacketHead, PACKET_HEAD_LEN),
                [this](const asio::error_code& error, size_t bytes_rcved)
                {
                    handleHeadReceiveFrom(error, bytes_rcved);
                });
        } else {
            fprintf(stderr, "Cannot set wanted events.(%d:%s)\n", error.value(), error.message().c_str());
            assert(false);
        }
    }

    void handleHeadReceiveFrom(const asio::error_code& error, size_t bytes_rcved) {
        assert(bytes_rcved == PACKET_HEAD_LEN);

        uint8_t major_type = *mRcvPacketHead;
        uint8_t minor_type = *(mRcvPacketHead + PACKET_MAJOR_TYPE_LEN);
        DDD("%s: handleHeadReceiveFrom rcved:%d, major_type, minor_type:(%d, %d)", __func__, (int)bytes_rcved, major_type, minor_type);

        uint8_t *packetSizePtr = mRcvPacketHead + PACKET_MAJOR_TYPE_LEN + PACKET_MINOR_TYPE_LEN;
        mRcvPacketBodyLen = *((uint64_t *)packetSizePtr);

        bool isWantedEvents = false;
        if ((GLPacketType)major_type == GLPacketType::CTRL_PACKET) {
            if ((GLCtrlType)minor_type == GLCtrlType::CLOSE_CTRL) {
                DDD("%s: try to close pipe server", __func__);
                mEmuglPipeServer->onGuestClose();
                delete this;
                return;
            } else if ((GLCtrlType)minor_type == GLCtrlType::SET_STATE_CTRL) {
                DDD("%s: try to set wanted events", __func__);
                isWantedEvents = true;
            } else {
                assert(false);
            }
        }

        if (mRcvPacketData== nullptr) {
            mRcvPacketDataCap= mRcvPacketBodyLen > CHANNEL_BUF_CAP ? mRcvPacketBodyLen : CHANNEL_BUF_CAP;
            mRcvPacketData = (uint8_t *)malloc(mRcvPacketDataCap);
        } else {
            if (mRcvPacketBodyLen > mRcvPacketDataCap) {
                mRcvPacketDataCap = 2 * mRcvPacketBodyLen;
                mRcvPacketData = (uint8_t *)realloc(mRcvPacketData, mRcvPacketDataCap);
            }
        }

        if (isWantedEvents) {
            asio::async_read(
                mSock,
                asio::buffer(mRcvPacketData, mRcvPacketBodyLen),
                [this](const asio::error_code& error, size_t bytes_rcvd) {
                    handleSetWantEventReceiveFrom(error, bytes_rcvd);
                });
        } else {
            asio::async_read(
                mSock,
                asio::buffer(mRcvPacketData, mRcvPacketBodyLen),
                [this](const asio::error_code& error, size_t bytes_rcvd) {
                    handleBodyReceiveFrom(error, bytes_rcvd);
                });
        }
    }

    void handleBodyReceiveFrom(const asio::error_code& error, size_t bytes_rcved) {
        DDD("handleBodyReceiveFrom body:%d,%d", (int)bytes_rcved, (int)mRcvPacketBodyLen);
        assert(bytes_rcved == mRcvPacketBodyLen);

        AndroidPipeBuffer pipeBuffer = {mRcvPacketData, mRcvPacketBodyLen};
        mEmuglPipeServer->onGuestSend(&pipeBuffer, 1);

        asio::async_read(
            mSock,
            asio::buffer(mRcvPacketHead, PACKET_HEAD_LEN),
            [this](const asio::error_code& error, size_t bytes_rcved) {
                handleHeadReceiveFrom(error, bytes_rcved);
            });
    }

private:
    AsioTCP::socket  mSock;
    EmuglPipeServer *mEmuglPipeServer;

    uint8_t  mRcvPacketHead[PACKET_HEAD_LEN] = {0};
    uint64_t mRcvPacketBodyLen    = 0;
    uint8_t  mRcvPacketDataCap    = 0;
    uint8_t *mRcvPacketData       = nullptr;
};

class EmuglPipeServerServer : public android::base::Thread {
public:
    EmuglPipeServerServer() : android::base::Thread(), mIoService(), mSocket(mIoService) {
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

    ~EmuglPipeServerServer() {
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
                    new EmuglSockPipe(std::move(mSocket));
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
    globalPipeServer = new EmuglPipeServerServer();
    globalPipeServer->start();

    printf("Start registerPipeServerService\n");

    registerGLProcessPipeService();
}

}  // namespace opengl
}  // namespace android

// Declared in android/opengles-pipe.h
void android_init_opengles_server_pipe() {
    android::opengl::registerPipeServerService();
}
