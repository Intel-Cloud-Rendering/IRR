#ifndef ASIO_SERVER_H
#define ASIO_SERVER_H
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

namespace irr {

  class AsioServer {
 public:
    AsioServer(boost::asio::io_service& io_service, short port);
    void run();
    void terminate();
 private:
    void do_accept();
    virtual void handle_accept() = 0;
    virtual void handle_terminate() = 0;
    short m_port;
    tcp::acceptor m_acceptor;
    bool m_terminate;
 protected:
    std::shared_ptr<tcp::socket> m_socket;
    /* use explicit strand as io_service.run called in main only */
    //std::shared_ptr<boost::asio::io_service::strand> m_strand;
  };
}

#endif
