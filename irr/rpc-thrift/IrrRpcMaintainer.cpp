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

#include <string>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TTransportException.h>
#include <thrift/TOutput.h>
#include "generated/IrrControl.h"
#include "overload/TIrrServer.h"
#include "overload/TIrrSocket.h"
#include "IrrControlHandler.h"
#include "IrrRpcMaintainer.h"

using std::string;
using ::apache::thrift::TProcessor;
using ::apache::thrift::stdcxx::shared_ptr;
using ::apache::thrift::GlobalOutput;
using ::IntelCloudRendering::IrrControlHandler;
using ::IntelCloudRendering::IrrControlProcessor;

using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

IrrRpcMaintainer::IrrRpcMaintainer(int port) {
    shared_ptr<IrrControlHandler> handler(new IrrControlHandler());
    shared_ptr<TProcessor> processor(new IrrControlProcessor(handler));
    shared_ptr<TServerTransport> serverTransport(new TServerSocket(port));
    shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
    shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());

    m_pServer = new TIrrServer(processor, serverTransport, transportFactory, protocolFactory);
}

IrrRpcMaintainer::IrrRpcMaintainer(shared_ptr<TProcessor> processor, int port) {
    shared_ptr<TServerTransport> serverTransport(new TServerSocket(port));
    shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
    shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());

    m_pServer = new TIrrServer(processor, serverTransport, transportFactory, protocolFactory);
}

IrrRpcMaintainer::~IrrRpcMaintainer() {
    delete m_pServer;
    m_pServer = nullptr;
}

int IrrRpcMaintainer::getFD() {
    auto transport = m_pServer->getServerTransport();
    return transport->getSocketFD();
}

int IrrRpcMaintainer::serve() {
    try {
        m_pServer->serve();
    } catch (TTransportException& ttx) {
        if (ttx.getType() == TTransportException::TIMED_OUT) {
            // Accept timeout - continue processing.
            return -ETIMEDOUT;
        } else if (ttx.getType() == TTransportException::END_OF_FILE
                   || ttx.getType() == TTransportException::INTERRUPTED) {
            // Server was interrupted.  This only happens when stopping.
            return -EINTR;
        } else {
            // All other transport exceptions are logged.
            // State of connection is unknown.  Done.
            string errStr = string("TServerTransport died: ") + ttx.what();
            GlobalOutput(errStr.c_str());
            return -EINVAL;
        }
    }

    return 0;
}
