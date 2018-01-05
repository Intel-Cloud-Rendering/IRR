#ifndef ASIO_SERVER_H
#define ASIO_SERVER_H
#include <boost/asio.hpp>
#include "AsioConnection.h"

using boost::asio::ip::tcp;

namespace irr {

  class AsioServer {
 public:
    AsioServer(boost::asio::io_service& io_service, short port);
    ~AsioServer();
    void run();
    void terminate();
 protected:
    virtual std::shared_ptr<AsioConnection> create_connection() = 0;
 private:
    void do_accept();
    void handle_accept(std::shared_ptr<AsioConnection>,
                       const boost::system::error_code&);
    short m_port;
    tcp::acceptor m_acceptor;
    std::vector<std::shared_ptr<AsioConnection>> connections;
  };
}

#endif
