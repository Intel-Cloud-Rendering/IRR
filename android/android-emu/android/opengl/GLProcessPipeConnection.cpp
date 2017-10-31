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
#include <string>

#include "android/opengl/GLProcessPipeConnection.h"
#include "android/base/threads/Thread.h"
#include "android/base/synchronization/Lock.h"
#include "android/asio/asio.hpp"
#include "android/connection/Connection.h"
#include "android/utils/gl_cmd_net_format.h"
#include "android/opengles.h"

using AsioIoService = asio::io_service;
using AsioTCP       = asio::ip::tcp;
using AutoLock      = android::base::AutoLock;

namespace android {
namespace opengl {

class GLProcessPipeConnectionBase {
public:
    unsigned short port() {
        // We use the "OpenGL port + 1" to avoid too much env var.
        return (connection::Connection::Config().port() + 1);
    }

    const asio::ip::address_v4& addr() {
        return connection::Connection::Config().addr();
    }
};

class GLProcessPipeConnectionServer : public GLProcessPipeConnectionBase, public android::base::Thread {
public:
    static GLProcessPipeConnectionServer& Instance() {
        static GLProcessPipeConnectionServer* server = nullptr;
        if (server == nullptr) {
            // Nobody can delete this instance, but that's fine since
            // we only have one instance which is expected to long live.
            server = new GLProcessPipeConnectionServer;
        }
        return *server;
    }

    virtual intptr_t main() override {
        AutoLog();
        startAccept();
        mIoService.run();
        fprintf(stderr, "GLProcessPipeConnectionServer:%s: should never be here\n", __func__);
        return 0;
    }

    ~GLProcessPipeConnectionServer() {
        AutoLog();
        mSocket.close();
        delete mAcceptor;
    }

private:
    GLProcessPipeConnectionServer() : android::base::Thread(), mIoService(), mSocket(mIoService) {
        AutoLog();
        AsioTCP::endpoint ep(AsioTCP::v4(), port());
        mAcceptor = new AsioTCP::acceptor(mIoService, ep);
        m_uniqueId = 0;

        // Start send thread
        start();
    }

    void startAccept() {
        AutoLog();
        DD("%s: waiting connect...", __func__);
        mAcceptor->async_accept(
            mSocket,
            [this](std::error_code ec) {
                acceptHandler(ec);
            });
    }

    void acceptHandler(const std::error_code& ec) {
        AutoLog();
        if (!ec) {
            D("%s, connected!", __func__);
            waitAndCleanup();
        } else {
            fprintf(stderr, "Cannot accept client.(%d:%s)\n", ec.value(), ec.message().c_str());
            assert(false);
        }
    }

    void waitAndCleanup() {
        AutoLog();
        asio::async_read(
            mSocket,
            asio::buffer(&m_uniqueId, sizeof(m_uniqueId)),
            [this](const asio::error_code& error, size_t bytes_rcved) {
                assert(bytes_rcved == sizeof(m_uniqueId));
                assert(m_uniqueId != 0);
                DD("%s: calling android_cleanupProcGLObjects(0x%lx)", __func__, m_uniqueId);
                android_cleanupProcGLObjects(m_uniqueId);
                m_uniqueId = 0;
                waitAndCleanup();
            });
        mIoService.run();
    }

    uint64_t            m_uniqueId;
    AsioTCP::acceptor*  mAcceptor;
    AsioIoService       mIoService;
    AsioTCP::socket     mSocket;
};


class GLProcessPipeConnectionClient : public GLProcessPipeConnectionBase {
public:
    static GLProcessPipeConnectionClient& Instance() {
        static GLProcessPipeConnectionClient* client = nullptr;
        if (client == nullptr) {
            // Nobody can delete this instance, but that's fine since
            // we only have one instance which is expected to long live.
            client = new GLProcessPipeConnectionClient;
        }
        return *client;
    }

    bool cleanupGL(const uint64_t upid) {
        AutoLog();
        AutoLock lock(mLock);
        if (!mConnected) {
            return false;
        }
        DD("%s: sending cleanupGL(0x%lx) to server", __func__, upid);
        assert(upid != 0);
        asio::error_code ec;
        asio::write(mSocket, asio::buffer(&upid, sizeof(upid)), ec);
        return true;
    }

    ~GLProcessPipeConnectionClient() {
        AutoLog();
        AutoLock lock(mLock);
        mConnected = false;
        mSocket.close();
    }

private:
    GLProcessPipeConnectionClient() : mIoService(), mSocket(mIoService), mLock() {
        AutoLog();
        AutoLock lock(mLock);
        mConnected = false;

        AsioTCP::endpoint ep(addr(), port());
        asio::error_code ec;
        mSocket.connect(ep, ec);
        if (!ec) {
            D("%s: connected!", __func__);
            mConnected = true;
        } else {
            fprintf(stderr, "Cannot connect to Server.(%d:%s)\n", ec.value(), ec.message().c_str());
            assert(false);
        }
    }

    bool                mConnected;
    AsioIoService       mIoService;
    AsioTCP::socket     mSocket;
    mutable android::base::Lock mLock;
};

void setupGLProcessPipeConnection()
{
    // I was thinking to enforce our client/server check to avoid logic typo,
    // however, currently we prefer to disable these check such that we can
    // boot original Qemu to investigate issues.
    if (connection::Connection::Config().server()) {
        GLProcessPipeConnectionServer::Instance();
    } else {
        GLProcessPipeConnectionClient::Instance();
    }
}

void cleanupGLProcObjOverNetwork(const uint64_t puid)
{
    assert(connection::Connection::Config().client());
    if (!GLProcessPipeConnectionClient::Instance().cleanupGL(puid)) {
        fprintf(stderr, "%s: cleanupGL send fail\n", __func__);
    }
}

}  // namespace opengl
}  // namespace android
