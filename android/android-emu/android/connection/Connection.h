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

#include <string>
#include <cassert>
#include <cstdlib>

#include "android/asio/asio.hpp"

namespace android {
namespace connection {

class Connection {
public:
    static Connection& Config() {
        static Connection c;
        return c;
    }

    bool server() const { return mServer; }
    bool client() const { return !mServer; }

    const asio::ip::address_v4& addr() const {
        return mAddr;
    }

    unsigned short port() {
        return mPort;
    }

private:
    static constexpr unsigned short default_port = 23234;

    Connection() {
        const char* client_env = std::getenv("render_client");
        const char* server_env = std::getenv("render_server");
        mServer = (server_env != nullptr || client_env == nullptr);

        const char* port = std::getenv("render_server_port");
        if (port != nullptr) {
            mPort = std::atoi(port);
            printf("%s: port %d\n", __func__, mPort);
        } else {
            mPort = default_port;
        }

        const char* addr = std::getenv("render_svr_hostname");
        if (addr != nullptr) {
            mAddr = asio::ip::address_v4::from_string(addr);
            printf("%s: hostname %s\n", __func__, addr);
        }
    }

    bool                    mServer;
    unsigned short          mPort;
    asio::ip::address_v4    mAddr;
};

}  // namespace connection
}  // namespace android

