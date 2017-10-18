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
#include "android/base/threads/Thread.h"
#include "android/opengles.h"
#include "android/opengles-pipe.h"
#include "android/opengl/GLProcessPipe.h"
#include "android/utils/gl_cmd_net_format.h"
#include "android/utils/system.h"
#include "android/utils/debug.h"
#include "android/base/synchronization/Lock.h"

#include <atomic>

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <memory>
#include <list>

#include "asio.hpp"

using ChannelBuffer = emugl::RenderChannel::Buffer;
using ChannelState  = emugl::RenderChannel::State;
using IoResult      = emugl::RenderChannel::IoResult;
using AsioIoService = asio::io_service;
using AsioTCP       = asio::ip::tcp;
using AutoLock      = android::base::AutoLock;

#define CHANNEL_BUF_CAP (512)

namespace android {
namespace opengl {

namespace {

class EmuglPipeClient : public AndroidPipe, public android::base::Thread {
public:
    typedef struct _EmuglSendBuffer {
        void *data;
        int   dataLen;
    } EmuglSendBuffer;

    //////////////////////////////////////////////////////////////////////////
    // The pipe service class for this implementation.
    class Service : public AndroidPipe::Service {
    public:
        Service() : AndroidPipe::Service("opengles") {}

        // Create a new EmuglPipe instance.
        virtual AndroidPipe* create(void* mHwPipe, const char* args) override {
            AutoLog();

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
        AutoLog();

        mIsWorking = false;

        const char* render_svr_hostname = getenv("render_svr_hostname");
        if (render_svr_hostname) {
            DDD("Render server hostname: %s", render_svr_hostname);
        } else {
            fprintf(stderr, "Cannot find render server hostname\n");
            return;
        }

        const char* render_svr_port = getenv("render_svr_port");
        if (render_svr_port) {
            DDD("Render server port: %s", render_svr_port);
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

        asio::async_read(
            mTcpSocket,
            asio::buffer(mRcvPacketHead, PACKET_HEAD_LEN),
            [this](const asio::error_code& error, size_t bytes_rcvd) {
                handleHeadReceiveFrom(error, bytes_rcvd);
            });

        // Start handling received/sended OpenGL-ES commands
        start();

        mIsWorking = true;
    }

    ~EmuglPipeClient() {
        AutoLog();

        if (!(mAsioIoService.stopped())) {
            mAsioIoService.stop();
        }

        if (mRcvPacketData != nullptr) {
            //TODO: We need to clarify the code
            //free(mRcvPacketData);
            //mRcvPacketData = nullptr;
        }

        // wait thread exit
        wait();
    }
    
    
    virtual intptr_t main() override {
        AutoLog();
        mAsioIoService.run();
        return 0;
    }

    //////////////////////////////////////////////////////////////////////////
    // Overriden AndroidPipe methods

    virtual void onGuestClose() override {
        AutoLog();

        mIsWorking = false;

        // Make sure there's no operation scheduled for this pipe instance to
        // run on the main thread.
        uint8_t sndBuf[10] = {0};
        int format_cmd_size = format_gl_ctrl_command(GLCtrlType::CLOSE_CTRL, sizeof(sndBuf), sndBuf);
        assert(format_cmd_size == sizeof(sndBuf));
        asio::error_code ec;
        DDD("%s: send %d bytes to rendering server", __func__, format_cmd_size);
        asio::write(mTcpSocket, asio::buffer(sndBuf, format_cmd_size), ec);
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
        AutoLog();

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
        AutoLog();

        // Consume the pipe's dataForReading, then put the next received data
        // piece there. Repeat until the buffers are full or we're out of data
        // in the channel.
        int len = 0;
        size_t buffOffset = 0;

        auto buff = buffers;
        const auto buffEnd = buff + numBuffers;
        while (buff != buffEnd) {
            const int spinCount = 30;
            for (int i = 0; i <= spinCount; i++, usleep(1)) {
                {
                    AutoLock lock(mLock);
                    if (mRcvPacketDataSize > 0) {
                        break;
                    }
                }

                sleep_ms(1);

                if (i == spinCount) {
                    if (len > 0) {
                        return len;
                    } else {
                        DD("%s: returning PIPE_ERROR_AGAIN", __func__);
                        return PIPE_ERROR_AGAIN;
                    }
                }
            }

            AutoLock lock(mLock);
            const size_t curSize = std::min(buff->size - buffOffset, mRcvPacketDataSize);
            memcpy(buff->data + buffOffset, mRcvPacketData + mRcvPacketDataOffset, curSize);

            len += curSize;
            mRcvPacketDataOffset += curSize;
            mRcvPacketDataSize -= curSize;
            assert(mRcvPacketDataSize >= 0);
            buffOffset += curSize;
            if (buffOffset == buff->size) {
                ++buff;
                buffOffset = 0;
            }
        }

        AutoLock lock(mLock);
        if (mRcvPacketDataSize > 0) {
            memmove(mRcvPacketData, mRcvPacketData + mRcvPacketDataOffset, mRcvPacketDataSize);
            mState |= (ChannelState::CanRead);
        } else {
            mState &= ~(ChannelState::CanRead);
        }
        mRcvPacketDataOffset = 0;

        /*
        // Update state
        this->onChannelHostEvent();
        */
        DD("%s: received %d bytes", __func__, (int)len);
        return len;
    }

    virtual int onGuestSend(const AndroidPipeBuffer* buffers,
                            int numBuffers) override {
        AutoLog();

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
        DDD("%s: send bytes(head, body):(%d,%d)", __func__, PACKET_HEAD_LEN, count);
        asio::write(mTcpSocket, asio::buffer(sndBuf, PACKET_HEAD_LEN + count), ec);
        if (ec) {
            fprintf(stderr, "Cannot send data to server.(%d:%s)\n", ec.value(), ec.message().c_str());
        }

        delete sndBuf;
        //sendToRenderingServer(sndBuf, PACKET_HEAD_LEN + count);

        return count;
    }

    virtual void onGuestWantWakeOn(int flags) override {
        AutoLog();
        DDD("%s: flags=%d", __func__, flags);

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
        DDD("%s: flags, wanted, state, available:%d, %d, %d, %d", __func__, flags, (int)wanted, (int)mState, (int)available);
        if (available != ChannelState::Empty) {
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
    void sendToRenderingServer(void *data, int dataLen) {
        AutoLog();

        DDD("%s: trying to send %d bytes to rendering server", __func__, dataLen);

        EmuglSendBuffer emuglSndBuf = {data, dataLen};
        {
            AutoLock lock(mSendBuffersLock);
            mSendBuffers.push_back(emuglSndBuf);
        }

        asio::error_code ec;
        // Kick off send to server
        handleSendDataToServer(ec, dataLen);
        assert(!ec);
    }

    void handleSendDataToServer(const asio::error_code& error, size_t bytes_snded) {
        AutoLog();

        EmuglSendBuffer emuglSndBuf;
        {
            AutoLock lock(mSendBuffersLock);
            if (mSendBuffers.empty()) {
                return;
            }
            emuglSndBuf = *(mSendBuffers.begin());
            mSendBuffers.pop_front();
        }

        void *data    = emuglSndBuf.data;
        int   dataLen = emuglSndBuf.dataLen;
        asio::async_write(
            mTcpSocket,
            asio::buffer(data, dataLen),
            [this, data](const asio::error_code& ec, std::size_t bytes_transferred) {
                // Release data memory
                free(data);

                android_tid_function_print(false, "async_write", "sock:%p send %lu bytes to rendering server", &mTcpSocket, bytes_transferred);
                if (ec) {
                    fprintf(stderr, "cannot send data to rendering server.(%d:%s)\n", ec.value(), ec.message().c_str());
                } else {
                    handleSendDataToServer(ec, bytes_transferred);
                }
            });
    }

    void handleBodyReceiveFrom(const asio::error_code& error, size_t bytes_rcved) {
        AutoLog();

        DDD("%s: error:%d, bytes received:%d, working:%d",
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
        AutoLog();

        DDD("%s: error:(%d,%s), bytes received:%d, working:%d",
            __func__,
            error.value(),
            error.message().c_str(),
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
            mRcvPacketDataOffset = 0;
            mRcvPacketDataSize = 0;
        } else {
            if (mRcvPacketBodyLen > (mRcvPacketDataCap - mRcvPacketDataOffset)) {
                mRcvPacketDataCap += (2 * mRcvPacketBodyLen);
                mRcvPacketData = (uint8_t *)realloc(mRcvPacketData, mRcvPacketDataCap);
            }
        }

        asio::async_read(
            mTcpSocket,
            asio::buffer(mRcvPacketData + mRcvPacketDataOffset, mRcvPacketBodyLen),
            [this](const asio::error_code& error, size_t bytes_rcved) {
                handleBodyReceiveFrom(error, bytes_rcved);
            });
    }

    void notifyRenderServerWantedEvents(int flags) {
        AutoLog();

        DDD("%s: set wanted events: %d", __func__, flags);

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
        assert(format_cmd_size == sizeof(sndBuf));

        asio::error_code ec;
        DDD("%s: send %d bytes to rendering server", __func__, format_cmd_size);
        asio::write(mTcpSocket, asio::buffer(sndBuf, format_cmd_size), ec);

        if (ec) {
            fprintf(stderr, "Cannot set channel state to server.(%d:%s)\n", ec.value(), ec.message().c_str());
            assert(false);
        }
    }

    void setChannelWantedState(ChannelState channelWantedState) {
        AutoLog();

        DDD("%s: set wanted state: %d", __func__, (int)channelWantedState);
        // Update local wanted events
        mWantedState |= channelWantedState;
        onChannelHostEvent();
    }

    // Called to signal the guest that read/write wake events occured.
    // Note: this can be called from either the guest or host render
    // thread.
    void signalState(ChannelState state) {
        AutoLog();

        int wakeFlags = 0;
        if ((state & ChannelState::CanRead) != 0) {
            wakeFlags |= PIPE_WAKE_READ;
        }
        if ((state & ChannelState::CanWrite) != 0) {
            wakeFlags |= PIPE_WAKE_WRITE;
        }
        if (wakeFlags != 0) {
            DD("%s: wakeFlags:%d", __func__, wakeFlags);
            this->signalWake(wakeFlags);
        }
    }

    // Called when an i/o event occurs on the render channel
    void onChannelHostEvent() {
        AutoLog();

        // NOTE: This is called from the host-side render thread.
        // but closeFromHost() and signalWake() can be called from
        // any thread.
        if ((mState & ChannelState::Stopped) != 0) {
            this->closeFromHost();
            return;
        }

        // The logic of notifyStateChangeLocked
        ChannelState available = mState & mWantedState;
        DDD("%s: (mState, mWantedState, available): (%d, %d, %d)", __func__, mState, mWantedState, (int)available);
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
    uint64_t mRcvPacketDataCap    = 0;
    uint8_t *mRcvPacketData       = nullptr;
    uint64_t mRcvPacketDataOffset = 0;
    uint64_t mDataForReadingLeft  = 0;

    AsioIoService   mAsioIoService;
    AsioTCP::socket mTcpSocket;

    mutable android::base::Lock mLock;

    std::list<EmuglSendBuffer>  mSendBuffers;
    mutable android::base::Lock mSendBuffersLock;

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
