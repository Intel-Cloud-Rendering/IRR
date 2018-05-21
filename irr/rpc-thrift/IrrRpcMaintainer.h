// Copyright 2018 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#ifndef IRRRPCMAINTAINER_H
#define IRRRPCMAINTAINER_H

#include <thrift/TProcessor.h>
#include <thrift/transport/TTransport.h>
#include <thrift/server/TServer.h>

class IrrRpcMaintainer {
public:
    IrrRpcMaintainer(int port);
    IrrRpcMaintainer(::apache::thrift::stdcxx::shared_ptr<::apache::thrift::TProcessor> processor, int port);
    ~IrrRpcMaintainer();
    int getFD();
    int serve();

private:
    ::apache::thrift::server::TServer *m_pServer;
};

#endif /* IRRRPCMAINTAINER_H */

