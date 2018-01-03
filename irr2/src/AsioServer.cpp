#include "AsioServer.h"
#include "RenderLog.h"

using namespace irr;

AsioServer::AsioServer(boost::asio::io_service& io_service, short port)
    : m_port(port),
      //m_socket(std::make_shared<tcp::socket>(io_service)),
      m_acceptor(io_service, tcp::endpoint(tcp::v4(), port)),
      m_terminate(false) {
  m_socket = std::make_shared<tcp::socket>(io_service);
}

void AsioServer::run() {
  do_accept();
}

void AsioServer::terminate() {
  /* no need to lock as only 1 place to write */
  handle_terminate();
  m_terminate = true;
  m_acceptor.close();
}

void AsioServer::do_accept() {
  m_acceptor.async_accept(
      *m_socket,
      [this](boost::system::error_code ec) {
        if (!ec)
        {
          handle_accept();
        }
        if (!m_terminate) {
          do_accept();
        }
        else {
          irr_log_info("exiting server on port: %d", m_port);
        }
      });
}
