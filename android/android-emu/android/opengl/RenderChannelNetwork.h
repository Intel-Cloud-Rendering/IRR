#pragma once

#include "android/utils/compiler.h"

#include "OpenglRender/RenderChannel.h"
#include "OpenglRender/BufferQueue.h"

#include "android/base/synchronization/Lock.h"

#include <stddef.h>
#include <stdint.h>
#include <errno.h>

#include <thread>

ANDROID_BEGIN_HEADER

using AutoLock        = android::base::AutoLock;
using Lock            = android::base::Lock;
using SockBuffer      = emugl::RenderChannel::Buffer;
using SockBufferQueue = emugl::BufferQueue;
using IoResult        = emugl::RenderChannel::IoResult;
using State           = emugl::RenderChannel::State;
using EventCallback   = emugl::RenderChannel::EventCallback;

namespace android {
namespace opengl {

class RenderChannelNetwork : public emugl::RenderChannel {
public:
    RenderChannelNetwork(char *hostName, uint16_t port);
    ~RenderChannelNetwork();

    /////////////////////////////////////////////////////////////////
    // RenderChannel overriden methods. These are called from the guest
    // client thread.

    // Set the event |callback| to be notified when the host changes the
    // state of the channel, according to the event mask provided by
    // setWantedEvents(). Call this function right after creating the
    // instance.
    virtual void setEventCallback(EventCallback&& callback) override final;

    // Set the mask of events the guest wants to be notified of from the
    // host thread.
    virtual void setWantedEvents(State state) override final;

    // Return the current channel state relative to the guest.
    virtual State state() const override final;

    // Try to send a buffer from the guest to the host render thread.
    virtual IoResult tryWrite(Buffer&& buffer) override final;

    // Try to read a buffer from the host render thread into the guest.
    virtual IoResult tryRead(Buffer* buffer) override final;

    // Close the channel from the guest.
    virtual void stop() override final;

    /////////////////////////////////////////////////////////////////
    // These functions are called from the host render thread.
    bool start();

    // Send a buffer to the guest, this call is blocking. On success,
    // move |buffer| into the channel and return true. On failure, return
    // false (meaning that the channel was closed).
    bool writeToGuest(Buffer&& buffer);

    // Read data from the guest. If |blocking| is true, the call will be
    // blocking. On success, move item into |*buffer| and return true. On
    // failure, return IoResult::Error to indicate the channel was closed,
    // or IoResult::TryAgain to indicate it was empty (this can happen only
    // if |blocking| is false).
    IoResult readFromGuest(Buffer* buffer, bool blocking);

    // Close the channel from the host.
    void stopFromHost();

private:
    int sendWorker();
    int receiveWorker();
    int rcvBufUntil(uint8_t *buf, int wantBufLen);

    void updateStateLocked();
    void notifyStateChangeLocked();

private:
    Lock             mSndDataLock;
    Lock             mRcvDataLock;
    Lock             mAPILock;

    SockBufferQueue  mSndBuffers;
    std::thread     *mSndThread;

    SockBufferQueue  mRcvBuffers;
    std::thread     *mRcvThread;

    int              mSockFd;
    char             mSockIP[512] = {0};
    int              mSockPort;
    bool             mStop;

    State mState        = State::Empty;
    State mWantedEvents = State::Empty;
    EventCallback mEventCallback;
};

}
}

ANDROID_END_HEADER

