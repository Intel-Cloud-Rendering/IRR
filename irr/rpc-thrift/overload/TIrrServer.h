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

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>
namespace apache {
    namespace thrift {
        namespace server {
            class TIrrServer : public TServer {
            public:
                TIrrServer(const stdcxx::shared_ptr<apache::thrift::TProcessorFactory>& processorFactory,
                           const stdcxx::shared_ptr<apache::thrift::transport::TServerTransport>& serverTransport,
                           const stdcxx::shared_ptr<apache::thrift::transport::TTransportFactory>& transportFactory,
                           const stdcxx::shared_ptr<apache::thrift::protocol::TProtocolFactory>& protocolFactory);

                TIrrServer(const stdcxx::shared_ptr<apache::thrift::TProcessor>& processor,
                           const stdcxx::shared_ptr<apache::thrift::transport::TServerTransport>& serverTransport,
                           const stdcxx::shared_ptr<apache::thrift::transport::TTransportFactory>& transportFactory,
                           const stdcxx::shared_ptr<apache::thrift::protocol::TProtocolFactory>& protocolFactory);

                TIrrServer(const stdcxx::shared_ptr<apache::thrift::TProcessorFactory>& processorFactory,
                           const stdcxx::shared_ptr<apache::thrift::transport::TServerTransport>& serverTransport,
                           const stdcxx::shared_ptr<apache::thrift::transport::TTransportFactory>& inputTransportFactory,
                           const stdcxx::shared_ptr<apache::thrift::transport::TTransportFactory>& outputTransportFactory,
                           const stdcxx::shared_ptr<apache::thrift::protocol::TProtocolFactory>& inputProtocolFactory,
                           const stdcxx::shared_ptr<apache::thrift::protocol::TProtocolFactory>& outputProtocolFactory);

                TIrrServer(const stdcxx::shared_ptr<apache::thrift::TProcessor>& processor,
                           const stdcxx::shared_ptr<apache::thrift::transport::TServerTransport>& serverTransport,
                           const stdcxx::shared_ptr<apache::thrift::transport::TTransportFactory>& inputTransportFactory,
                           const stdcxx::shared_ptr<apache::thrift::transport::TTransportFactory>& outputTransportFactory,
                           const stdcxx::shared_ptr<apache::thrift::protocol::TProtocolFactory>& inputProtocolFactory,
                           const stdcxx::shared_ptr<apache::thrift::protocol::TProtocolFactory>& outputProtocolFactory);

                virtual ~TIrrServer();
                void serve();

            private:
                void start();
            };
        }
    }
}
