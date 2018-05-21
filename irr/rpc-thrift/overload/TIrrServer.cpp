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

#include "TIrrServer.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using stdcxx::shared_ptr;
using std::string;

template <typename T>
static void releaseOneDescriptor(const string& name, T& pTransport) {
    if (pTransport) {
        try {
            pTransport->close();
        } catch (const TTransportException& ttx) {
            string errStr = string("TServerFramework " + name + " close failed: ") + ttx.what();
            GlobalOutput(errStr.c_str());
        }
    }
}

TIrrServer::TIrrServer(
    const stdcxx::shared_ptr<apache::thrift::TProcessorFactory>& processorFactory,
    const stdcxx::shared_ptr<apache::thrift::transport::TServerTransport>& serverTransport,
    const stdcxx::shared_ptr<apache::thrift::transport::TTransportFactory>& transportFactory,
    const stdcxx::shared_ptr<apache::thrift::protocol::TProtocolFactory>& protocolFactory)
: TServer(processorFactory, serverTransport, transportFactory, protocolFactory) {
    start();
}

TIrrServer::TIrrServer(
    const stdcxx::shared_ptr<apache::thrift::TProcessor>& processor,
    const stdcxx::shared_ptr<apache::thrift::transport::TServerTransport>& serverTransport,
    const stdcxx::shared_ptr<apache::thrift::transport::TTransportFactory>& transportFactory,
    const stdcxx::shared_ptr<apache::thrift::protocol::TProtocolFactory>& protocolFactory)
: TServer(processor, serverTransport, transportFactory, protocolFactory) {
    start();
}

TIrrServer::TIrrServer(
    const stdcxx::shared_ptr<apache::thrift::TProcessorFactory>& processorFactory,
    const stdcxx::shared_ptr<apache::thrift::transport::TServerTransport>& serverTransport,
    const stdcxx::shared_ptr<apache::thrift::transport::TTransportFactory>& inputTransportFactory,
    const stdcxx::shared_ptr<apache::thrift::transport::TTransportFactory>& outputTransportFactory,
    const stdcxx::shared_ptr<apache::thrift::protocol::TProtocolFactory>& inputProtocolFactory,
    const stdcxx::shared_ptr<apache::thrift::protocol::TProtocolFactory>& outputProtocolFactory)
: TServer(processorFactory,
                   serverTransport,
                   inputTransportFactory,
                   outputTransportFactory,
                   inputProtocolFactory,
                   outputProtocolFactory) {
    start();
}

TIrrServer::TIrrServer(
    const stdcxx::shared_ptr<apache::thrift::TProcessor>& processor,
    const stdcxx::shared_ptr<apache::thrift::transport::TServerTransport>& serverTransport,
    const stdcxx::shared_ptr<apache::thrift::transport::TTransportFactory>& inputTransportFactory,
    const stdcxx::shared_ptr<apache::thrift::transport::TTransportFactory>& outputTransportFactory,
    const stdcxx::shared_ptr<apache::thrift::protocol::TProtocolFactory>& inputProtocolFactory,
    const stdcxx::shared_ptr<apache::thrift::protocol::TProtocolFactory>& outputProtocolFactory)
: TServer(processor,
                   serverTransport,
                   inputTransportFactory,
                   outputTransportFactory,
                   inputProtocolFactory,
                   outputProtocolFactory) {
    start();
}

TIrrServer::~TIrrServer(){
}

void TIrrServer::serve() {
    shared_ptr<TTransport> client;
    shared_ptr<TTransport> inputTransport;
    shared_ptr<TTransport> outputTransport;
    shared_ptr<TProtocol> inputProtocol;
    shared_ptr<TProtocol> outputProtocol;

    try {
        // Dereference any resources from any previous client creation
        // such that a blocking accept does not hold them indefinitely.
        outputProtocol.reset();
        inputProtocol.reset();
        outputTransport.reset();
        inputTransport.reset();
        client.reset();

        client = serverTransport_->accept();

        inputTransport = inputTransportFactory_->getTransport(client);
        outputTransport = outputTransportFactory_->getTransport(client);
        if (!outputProtocolFactory_) {
            inputProtocol = inputProtocolFactory_->getProtocol(inputTransport, outputTransport);
            outputProtocol = inputProtocol;
        } else {
            inputProtocol = inputProtocolFactory_->getProtocol(inputTransport);
            outputProtocol = outputProtocolFactory_->getProtocol(outputTransport);
        }

        TConnectedClient cc(getProcessor(inputProtocol, outputProtocol, client),
                            inputProtocol, outputProtocol, eventHandler_, client);
        cc.run();
    } catch (TTransportException& ttx) {
        releaseOneDescriptor("inputTransport", inputTransport);
        releaseOneDescriptor("outputTransport", outputTransport);
        releaseOneDescriptor("client", client);
        throw ttx;
    }
}
void TIrrServer::start() {
    // Start the server listening
    serverTransport_->listen();
    // Run the preServe event to indicate server is now listening
    // and that it is safe to connect.
    if (eventHandler_) {
        eventHandler_->preServe();
    }
}

