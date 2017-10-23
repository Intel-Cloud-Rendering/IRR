#include "RenderChannelNetwork.h"

#include "android/utils/sockets.h"
#include "android/utils/ipaddr.h"
#include "android/utils/gl_cmd_net_format.h"

#include <string.h>

namespace android {
namespace opengl {

#if defined(_WIN32) && !defined(_WIN64)
    static constexpr size_t kInstanceToRenderQueueCapacity = 32U;
#else
    static constexpr size_t kInstanceToRenderQueueCapacity = 1024U;
#endif

    static constexpr size_t kRenderToInstanceQueueCapacity = 16U;

RenderChannelNetwork::RenderChannelNetwork(char *hostName, uint16_t port)
        : mSndDataLock(),
          mRcvDataLock(),
          mAPILock(),
          mSndBuffers(kInstanceToRenderQueueCapacity, mSndDataLock),
          mSndThread(nullptr),
          mRcvBuffers(kRenderToInstanceQueueCapacity, mRcvDataLock),
          mRcvThread(nullptr) {
    //TODO: Check buffer overflow
    strcpy(mSockIP, hostName);
    mSockPort = port;
    mSockFd   = -1;

    mStop = false;
    mEventCallback = nullptr;
}

RenderChannelNetwork::~RenderChannelNetwork() {
}

void RenderChannelNetwork::setEventCallback(EventCallback&& callback) {
    mEventCallback = std::move(callback);
}

void RenderChannelNetwork::setWantedEvents(State state) {
    AutoLock lock(mAPILock);

    mWantedEvents |= state;
    notifyStateChangeLocked();
}

bool RenderChannelNetwork::start() {
    if (mSockFd == -1) {
        mSockFd = socket_network_client(mSockIP, mSockPort, SOCKET_STREAM);
        if (mSockFd == -1) {
            fprintf(stderr, "%s: cannot connect to rendering server.(%s)\n", __func__, errno_str);
            return false;
        }
    }

    mSndThread = new std::thread([this] {this->sendWorker();});
    mSndThread->detach();

    mRcvThread = new std::thread([this] {this->receiveWorker();});
    mRcvThread->detach();

    return true;
}

void RenderChannelNetwork::stop() {
    mSndBuffers.closeLocked();
    mRcvBuffers.closeLocked();

    mStop = true;

    mSndThread->join();
    mRcvThread->join();

    delete mSndThread;
    delete mRcvThread;
}

IoResult RenderChannelNetwork::tryWrite(SockBuffer&& buffer) {
    AutoLock lock(mAPILock);
    auto result = mSndBuffers.tryPushLocked(std::move(buffer));
    return result;
}

IoResult RenderChannelNetwork::tryRead(SockBuffer* buffer) {
    AutoLock lock(mAPILock);
    auto result = mRcvBuffers.tryPopLocked(buffer);
    return result;
}

int RenderChannelNetwork::sendWorker() {
    while (!mStop) {
        SockBuffer sockBuffer;
        readFromGuest(&sockBuffer, true);

        int writePos = 0;
        int nLeft = sockBuffer.size();
        while (nLeft > 0) {
            int ret = socket_send(mSockFd, sockBuffer.data() + writePos, nLeft);
            if (ret == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    continue;
                } else {
                    fprintf(stderr, "%s: cannot send %lu bytes data to renderint server.(%s)\n", __func__, sockBuffer.size(), errno_str);
                    return -1;
                }
            }

            writePos += ret;
            nLeft    -= ret;
        }
    }

    return 0;
}

int RenderChannelNetwork::receiveWorker() {
    GLCmdPacketHead headBuf = {0};
    while (!mStop) {
        int wantLen = sizeof(GLCmdPacketHead);
        int ret = rcvBufUntil((uint8_t *)(&headBuf), wantLen);
        if (ret == -1) {
            return ret;
        }

        wantLen = headBuf.packet_body_size;
        SockBuffer sockBuffer;
        sockBuffer.resize_noinit(wantLen);
        auto ptr = sockBuffer.data();
        ret = rcvBufUntil((uint8_t *)ptr, wantLen);
        assert(ret == wantLen);
        if (ret == -1) {
            return ret;
        }

        bool retVal;
        do {
            retVal = writeToGuest(std::move(sockBuffer));
        } while (!retVal);
    }

    return 0;
}

int RenderChannelNetwork::rcvBufUntil(uint8_t *buf, int wantBufLen) {
    int totalLen = 0;
    while ((totalLen < wantBufLen) && (!mStop)) {
        int ret = socket_recv(mSockFd, (buf + totalLen), wantBufLen - totalLen);
        if (ret == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            } else {
                fprintf(stderr, "%s: cannot receive %d bytes data from renderint server.(%s)\n", __func__, wantBufLen, errno_str);
                return -1;
            }
        }
        totalLen += ret;
    }
    return totalLen;
}

bool RenderChannelNetwork::writeToGuest(Buffer&& buffer) {
    AutoLock lock(mAPILock);

    IoResult result = mRcvBuffers.pushLocked(std::move(buffer));
    updateStateLocked();
    notifyStateChangeLocked();
    return result == IoResult::Ok;
}

IoResult RenderChannelNetwork::readFromGuest(Buffer* buffer, bool blocking) {
    AutoLock lock(mAPILock);

    IoResult result;
    if (blocking) {
        result = mSndBuffers.popLocked(buffer);
    } else {
        result = mSndBuffers.tryPopLocked(buffer);
    }
    updateStateLocked();
    notifyStateChangeLocked();
    return result;
}

void RenderChannelNetwork::stopFromHost() {
    AutoLock lock(mAPILock);

    mRcvBuffers.closeLocked();
    mSndBuffers.closeLocked();
    mState |= State::Stopped;
    notifyStateChangeLocked();
}

void RenderChannelNetwork::updateStateLocked() {
    State state = RenderChannel::State::Empty;

    if (mRcvBuffers.canPopLocked()) {
        state |= State::CanRead;
    }
    if (mSndBuffers.canPushLocked()) {
        state |= State::CanWrite;
    }
    if (mRcvBuffers.isClosedLocked()) {
        state |= State::Stopped;
    }
    mState = state;
}

void RenderChannelNetwork::notifyStateChangeLocked() {
    State available = mState & mWantedEvents;
    if (available != 0) {
        D("callback with %d", (int)available);
        mWantedEvents &= ~mState;
        mEventCallback(available);
    }
}

}
}

