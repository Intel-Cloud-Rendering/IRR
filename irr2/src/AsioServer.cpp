#include "AsioServer.h"
#include "RenderLog.h"

using namespace irr;

AsioServer::AsioServer(boost::asio::io_service& io_service, short port)
    : m_port(port),
      m_acceptor(io_service, tcp::endpoint(tcp::v4(), port)) {
}

AsioServer::~AsioServer() {
  m_acceptor.close();
}

void AsioServer::run() {
  do_accept();
}

void AsioServer::terminate() {
  for (std::vector<std::shared_ptr<AsioConnection>>::iterator it = m_connections.begin();
       it != m_connections.end(); it++) {
    (*it)->terminate();
  }
}

void AsioServer::handle_accept(std::shared_ptr<AsioConnection> connection,
                                 const boost::system::error_code& error) {
  if (!error) {
    irr_log_info("accepted");
    connection->start();
    m_connections.push_back(connection);
    do_accept();
  }
}
void AsioServer::do_accept() {
  std::shared_ptr<AsioConnection> connection = create_connection();
  m_acceptor.async_accept(connection->socket(),
                          boost::bind(&AsioServer::handle_accept, this,
                                      connection, boost::asio::placeholders::error));

  for (std::vector<std::shared_ptr<AsioConnection>>::iterator it = m_connections.begin();
       it != m_connections.end(); it++) {
    if ((*it)->is_terminated()) {
      (*it)->terminate();
    }
  }
}
