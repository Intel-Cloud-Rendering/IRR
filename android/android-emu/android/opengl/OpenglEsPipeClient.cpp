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
#include "android/opengl/OpenglEsPipeClient.h"

#include "android/base/async/Looper.h"
#include "android/opengles.h"
#include "android/opengles-pipe.h"
#include "android/opengl/GLProcessPipe.h"
#include "android/utils/gl_cmd_net_format.h"
#include "android/base/synchronization/Lock.h"

#include <atomic>

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <memory>
#include <thread>

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
using AutoLock = android::base::AutoLock;


#define CHANNEL_BUF_CAP (512)

namespace android {
namespace opengl {

namespace {

class EmuglPipeClient : public AndroidPipe {
public:
    //////////////////////////////////////////////////////////////////////////
    // The pipe service class for this implementation.
    class Service : public AndroidPipe::Service {
    public:
        Service() : AndroidPipe::Service("opengles") {}

        // Create a new EmuglPipe instance.
        virtual AndroidPipe* create(void* mHwPipe, const char* args) override {
            EmuglPipeClient* pipe = new EmuglPipeClient(mHwPipe, this);
            if (!pipe->mIsWorking) {
                delete pipe;
                pipe = nullptr;
            }
            return pipe;
        }

        // Really cannot save/load these pipes' state.
        virtual bool canLoad() const override { return false; }
    };

    /////////////////////////////////////////////////////////////////////////
    // Constructor, check that |mIsWorking| is true after this call to verify
    // that everything went well.
    EmuglPipeClient(void* hwPipe, Service* service) :
        AndroidPipe(hwPipe, service),
        mAsioIoService(),
        mTcpSocket(mAsioIoService),
        mLock() {

        mIsWorking = false;

        const char* render_svr_hostname = getenv("render_svr_hostname");
        if (render_svr_hostname) {
            DDD("Render server hostname: %s\n", render_svr_hostname);
        } else {
            fprintf(stderr, "Cannot find render server hostname\n");
            return;
        }

        const char* render_svr_port = getenv("render_svr_port");
        if (render_svr_port) {
            DDD("Render server port: %s\n", render_svr_port);
        } else {
            fprintf(stderr, "Cannot find render server port\n");
            return;
        }

        AsioTCP::resolver resolver(mAsioIoService);
        AsioTCP::resolver::query query(render_svr_hostname, render_svr_port);

        AsioTCP::resolver::iterator endpoint_iterator = resolver.resolve(query);
        asio::error_code ec;
        asio::connect(mTcpSocket, endpoint_iterator, ec);
        if (ec) {
            fprintf(stderr, "Cannot connect to server.(%d:%s)\n", ec.value(), ec.message().c_str());
            return;
        }

        mRcvThread = std::thread([this](){ mAsioIoService.run(); });
        mIsWorking = true;

        asio::async_read(
            mTcpSocket,
            asio::buffer(mRcvPacketHead, PACKET_HEAD_LEN),
            [this](const asio::error_code& error, size_t bytes_rcvd) {
                handleHeadReceiveFrom(error, bytes_rcvd);
            });
    }

    ~EmuglPipeClient() {
        if (!(mAsioIoService.stopped())) {
            mAsioIoService.stop();
        }

        if (mRcvPacketData != nullptr) {
            free(mRcvPacketData);
            mRcvPacketData = nullptr;
        }

        mRcvThread.join();
    }

    //////////////////////////////////////////////////////////////////////////
    // Overriden AndroidPipe methods

    virtual void onGuestClose() override {
        D("%s", __func__);
        mIsWorking = false;

        // Make sure there's no operation scheduled for this pipe instance to
        // run on the main thread.
        uint8_t sndBuf[10] = {0};
        int ret = format_gl_ctrl_command(GLCtrlType::CLOSE_CTRL, sizeof(sndBuf), sndBuf);
        assert(ret > 0);
        asio::error_code ec;
        mTcpSocket.send(asio::buffer(sndBuf, ret), 0, ec);
        if (ec) {
            fprintf(stderr, "Cannot send [close] to server.(%d:%s)\n", ec.value(), ec.message().c_str());
        }

        mTcpSocket.close();

        // Update state
        mState |= ChannelState::Stopped;

        abortPendingOperation();
        delete this;
    }

    virtual unsigned onGuestPoll() override {
        DD("%s", __func__);

        unsigned ret = 0;
        if (mDataForReadingLeft > 0) {
            ret |= PIPE_POLL_IN;
        }

        if ((mState & ChannelState::CanRead) != 0) {
            ret |= PIPE_POLL_IN;
        }
        if ((mState & ChannelState::CanWrite) != 0) {
            ret |= PIPE_POLL_OUT;
        }
        if ((mState & ChannelState::Stopped) != 0) {
            ret |= PIPE_POLL_HUP;
        }

        DD("%s: returning %d", __func__, ret);
        return ret;
    }

    virtual int onGuestRecv(AndroidPipeBuffer* buffers, int numBuffers) override {
        DD("%s", __func__);

        // Consume the pipe's dataForReading, then put the next received data
        // piece there. Repeat until the buffers are full or we're out of data
        // in the channel.
        int len = 0;
        size_t buffOffset = 0;

        auto buff = buffers;
        const auto buffEnd = buff + numBuffers;
        while (buff != buffEnd) {
            const int spinCount = 20;
            for (int i = 0; i <= spinCount; i++, usleep(1)) {
                AutoLock lock(mLock);
                if (mRcvPacketDataSize > 0) {
                    break;
                }
                if (i == spinCount) {
                    DD("%s: returning PIPE_ERROR_AGAIN", __func__);
                    return PIPE_ERROR_AGAIN;
                }
            }

            AutoLock lock(mLock);
            const size_t curSize = std::min(buff->size - buffOffset, mRcvPacketDataSize);
            memcpy(buff->data + buffOffset, mRcvPacketData + mRcvPacketDataOffset, curSize);

            len += curSize;
            mRcvPacketDataOffset += curSize;
            mRcvPacketDataSize -= curSize;
            buffOffset += curSize;
            if (buffOffset == buff->size) {
                ++buff;
                buffOffset = 0;
            }
        }

        AutoLock lock(mLock);
        if (mRcvPacketDataSize > 0) {
            memmove(mRcvPacketData, mRcvPacketData + mRcvPacketDataOffset, mRcvPacketDataSize);
            mRcvPacketDataOffset = 0;
            mState |= (ChannelState::CanRead);
        } else {
            mState &= ~(ChannelState::CanRead);
        }

        // Update state
        this->onChannelHostEvent();
        DD("%s: received %d bytes", __func__, (int)len);
        return len;
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
        uint8_t *sndBuf = new uint8_t[PACKET_HEAD_LEN + count];
        uint8_t *sndBufData = sndBuf + PACKET_HEAD_LEN;
        for (int n = 0; n < numBuffers; ++n) {
            memcpy(sndBufData, buffers[n].data, buffers[n].size);
            sndBufData += buffers[n].size;
        }

        int offset = 0;
        *sndBuf = (uint8_t)GLPacketType::DATA_PACKET;
        offset += PACKET_MAJOR_TYPE_LEN;

        *(sndBuf + offset) = 0;
        offset += PACKET_MINOR_TYPE_LEN;

        *((uint64_t *)(sndBuf + offset)) = count;

        asio::error_code ec;
        DDD("%s: send bytes(head, body):(%d,%d)\n", __func__, PACKET_HEAD_LEN, count);
        mTcpSocket.send(asio::buffer(sndBuf, PACKET_HEAD_LEN + count), 0, ec);
        if (ec) {
            fprintf(stderr, "Cannot send data to server.(%d:%s)\n", ec.value(), ec.message().c_str());
        }

        delete sndBuf;
        return count;
    }

    virtual void onGuestWantWakeOn(int flags) override {
        DD("%s: flags=%d", __func__, flags);

        // Notify server wanted flags
        notifyRenderServerWantedEvents(flags);

        // Translate |flags| into ChannelState flags.
        ChannelState wanted = ChannelState::Empty;
        if (flags & PIPE_WAKE_READ) {
            wanted |= ChannelState::CanRead;
        }
        if (flags & PIPE_WAKE_WRITE) {
            wanted |= ChannelState::CanWrite;
        }

        // Signal events that are already available now.
        ChannelState available = mState & wanted;
        DD("%s: flags, wanted, state, available:%d, %d, %d, %d\n", __func__, flags, (int)wanted, (int)mState, (int)available);
        if (available != ChannelState::Empty) {
            DDD("%s: signaling events %d", __func__, (int)available);
            signalState(available);
            wanted &= ~available;
        }

        // Ask the channel to be notified of remaining events.
        if (wanted != ChannelState::Empty) {
            DDD("%s: waiting for events %d", __func__, (int)wanted);
            setChannelWantedState(wanted);
        }
    }

private:
    void handleBodyReceiveFrom(const asio::error_code& error, size_t bytes_rcved) {
        DDD("%s: error:%d, bytes received:%d, working:%d\n",
            __func__,
            error.value(),
            (int)bytes_rcved,
            (int)mIsWorking);
        if (!mIsWorking)
            return;

        assert(bytes_rcved == mRcvPacketBodyLen);

        // Update state
        AutoLock lock(mLock);
        mRcvPacketDataSize += bytes_rcved;
        mState |= ChannelState::CanRead;

        // Try to notify guest
        this->onChannelHostEvent();

        asio::async_read(
            mTcpSocket,
            asio::buffer(mRcvPacketHead, PACKET_HEAD_LEN),
            [this](const asio::error_code& error, size_t bytes_rcved) {
                handleHeadReceiveFrom(error, bytes_rcved);
            });
    }

    void handleHeadReceiveFrom(const asio::error_code& error, size_t bytes_rcved) {
        DDD("%s: error:%d, bytes received:%d, working:%d\n",
            __func__,
            error.value(),
            (int)bytes_rcved,
            (int)mIsWorking);
        if (!mIsWorking)
            return;

        assert(bytes_rcved == PACKET_HEAD_LEN);

        uint8_t major_type = *mRcvPacketHead;
        uint8_t minor_type = *(mRcvPacketHead + PACKET_MAJOR_TYPE_LEN);
        if ((major_type != GLPacketType::DATA_PACKET) || (minor_type != 0)) {
            fprintf(stderr, "Invalid packet type\n");
            assert(false);
            return;
        }

        AutoLock lock(mLock);
        uint8_t *packetSizePtr = mRcvPacketHead + PACKET_MAJOR_TYPE_LEN + PACKET_MINOR_TYPE_LEN;
        mRcvPacketBodyLen = *((uint64_t *)packetSizePtr);
        if (mRcvPacketData == nullptr) {
            mRcvPacketDataCap = mRcvPacketBodyLen > CHANNEL_BUF_CAP ? mRcvPacketBodyLen : CHANNEL_BUF_CAP;
            mRcvPacketData = (uint8_t *)malloc(mRcvPacketDataCap);
        } else {
            if (mRcvPacketBodyLen > (mRcvPacketDataCap - mRcvPacketDataOffset)) {
                mRcvPacketDataCap += 2 * mRcvPacketBodyLen;
                mRcvPacketData = (uint8_t *)realloc(mRcvPacketData, mRcvPacketDataCap);
            }
        }

        asio::async_read(
            mTcpSocket,
            asio::buffer(mRcvPacketData + mRcvPacketDataSize, mRcvPacketBodyLen),
            [this](const asio::error_code& error, size_t bytes_rcved) {
                handleBodyReceiveFrom(error, bytes_rcved);
            });
    }

    void notifyRenderServerWantedEvents(int flags) {
        DDD("%s: set wanted events: %d\n", __func__, flags);

        // Notify rendering server
        uint8_t sndBuf[14] = {0};
        uint8_t majorType = (uint8_t)GLPacketType::CTRL_PACKET;
        uint8_t minorType = (uint8_t)GLCtrlType::SET_STATE_CTRL;
        int format_cmd_size = format_gl_generic_command(
            majorType,
            minorType,
            sizeof(int),
            (uint8_t *)(&flags),
            sizeof(sndBuf),
            sndBuf);
        assert(format_cmd_size > 0);
        asio::error_code ec;
        mTcpSocket.send(asio::buffer(sndBuf, format_cmd_size), 0, ec);
        if (ec) {
            fprintf(stderr, "Cannot set channel state to server.(%d:%s)\n", ec.value(), ec.message().c_str());
            assert(false);
        }
    }

    void setChannelWantedState(ChannelState channelWantedState) {
        DDD("%s: set wanted state: %d\n", __func__, (int)channelWantedState);
        // Update local wanted events
        mWantedState |= channelWantedState;
        onChannelHostEvent();
    }

    // Called to signal the guest that read/write wake events occured.
    // Note: this can be called from either the guest or host render
    // thread.
    void signalState(ChannelState state) {
        int wakeFlags = 0;
        if ((state & ChannelState::CanRead) != 0) {
            wakeFlags |= PIPE_WAKE_READ;
        }
        if ((state & ChannelState::CanWrite) != 0) {
            wakeFlags |= PIPE_WAKE_WRITE;
        }
        if (wakeFlags != 0) {
            DD("%s: wakeFlags:%d\n", __func__, wakeFlags);
            this->signalWake(wakeFlags);
        }
    }

    // Called when an i/o event occurs on the render channel
    void onChannelHostEvent() {
        D("%s: events %d", __func__, (int)mState);
        // NOTE: This is called from the host-side render thread.
        // but closeFromHost() and signalWake() can be called from
        // any thread.
        if ((mState & ChannelState::Stopped) != 0) {
            this->closeFromHost();
            return;
        }

        // The logic of notifyStateChangeLocked
        ChannelState available = mState & mWantedState;
        DDD("%s: (mState, mWantedEvents, available): (%d, %d, %d)\n", __func__, mState, mWantedState, (int)available);
        if (available != ChannelState::Empty) {
            D("%s: callback with %d", __func__, (int)available);
            // Update wanted events
            mWantedState &= ~mState;
            signalState(mState);
        }
    }

    // Set to |true| if the pipe is in working state, |false| means we're not
    // initialized or the pipe is closed.
    bool         mIsWorking   = false;
    ChannelState mState       = ChannelState::CanWrite;
    ChannelState mWantedState = ChannelState::Empty;

    bool     mRcvHead = true;
    uint8_t  mRcvPacketHead[PACKET_HEAD_LEN] = {0};
    uint64_t mRcvPacketBodyLen    = 0;
    uint64_t mRcvPacketDataSize   = 0;
    uint8_t  mRcvPacketDataCap    = 0;
    uint8_t *mRcvPacketData       = nullptr;
    uint64_t mRcvPacketDataOffset = 0;
    size_t   mDataForReadingLeft  = 0;

    AsioIoService   mAsioIoService;
    AsioTCP::socket mTcpSocket;
    std::thread     mRcvThread;

    mutable android::base::Lock mLock;

    DISALLOW_COPY_ASSIGN_AND_MOVE(EmuglPipeClient);
};

}  // namespace

void registerPipeClientService() {
    android::AndroidPipe::Service::add(new EmuglPipeClient::Service());
    registerGLProcessPipeService();
}

}  // namespace opengl
}  // namespace android

// Declared in android/opengles-pipe.h
void android_init_opengles_client_pipe() {
    android::opengl::registerPipeClientService();
}
