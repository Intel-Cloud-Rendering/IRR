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
    RenderSender(std::shared_ptr<tcp::socket> socket, std::shared_ptr<RenderChannel> channel);
    RenderSender(const RenderSender&) = delete;
    RenderSender(RenderSender&&);
    RenderSender& operator=(const RenderSender&) = delete;
    RenderSender& operator=(RenderSender&&) = delete;
    ~RenderSender();
    virtual void process();
    void terminate();
 private:
    std::shared_ptr<RenderChannel> m_channel;
    std::shared_ptr<tcp::socket> m_socket;
    bool m_terminate;
  };
}
#endif
