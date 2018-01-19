#ifndef ASIO_CONNECTION_H
#define ASIO_CONNECTION_H
#include <boost/asio.hpp>
#include <memory>
#include "RenderThread.h"

using boost::asio::ip::tcp;

namespace irr {

  class AsioConnection : public RenderThread {
 public:
    AsioConnection()
      : m_io_service(),
        m_socket(m_io_service),
        m_is_terminated(false) {
    }

    tcp::socket& socket() {
      return m_socket;
    }

    virtual void handle_terminate() = 0;
    virtual void handle_process() = 0;

    virtual void process() {
      handle_process();
      m_io_service.run();
    }

    void terminate() {
      m_io_service.post([this] () {
          m_socket.close();
        });
      handle_terminate();
    }

    bool is_terminated() {
      return m_is_terminated;
    }

 protected:
    bool m_is_terminated;
    boost::asio::io_service m_io_service;
    tcp::socket m_socket;
  };
}

#endif
