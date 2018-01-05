#ifndef RENDER_SENDER_H
#define RENDER_SENDER_H
#include <memory>
#include <boost/asio.hpp>
#include "RenderChannel.h"
#include "RenderThread.h"

using boost::asio::ip::tcp;

namespace irr {

  class RenderSender : public RenderThread {
 public:
    RenderSender(tcp::socket&, RenderChannel&);
    RenderSender(const RenderSender&) = delete;
    RenderSender(RenderSender&&);
    RenderSender& operator=(const RenderSender&) = delete;
    RenderSender& operator=(RenderSender&&) = delete;
    ~RenderSender();
    virtual void process();
    void terminate();
 private:
    RenderChannel& m_channel;
    tcp::socket& m_socket;
    bool m_terminate;
  };
}
#endif
