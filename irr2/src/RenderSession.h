#ifndef RENDER_SESSION_H
#define RENDER_SESSION_H
#include <boost/asio.hpp>
#include <memory>
#include "RenderReceiver.h"
#include "RenderChannel.h"
#include "RenderHandler.h"
#include "RenderSender.h"

using boost::asio::ip::tcp;

/*
 * ----------> [receiver] -----------> 
 *   <socket>              <channel>    [handler]
 * <----------  [sender]  <----------- 
 */

namespace irr {

  class RenderSession {
 public:
    RenderSession(std::shared_ptr<tcp::socket> socket);
    RenderSession(const RenderSession&) = delete;
    RenderSession(RenderSession&&);
    RenderSession& operator=(const RenderSession&) = delete;
    RenderSession& operator=(RenderSession&&) = delete;
    ~RenderSession();
    void terminate() {
      m_handler.terminate();
      m_sender.terminate();
    }
    void join_all() {
      m_handler.join();
    }
 private:
    std::shared_ptr<RenderChannel> m_channel;
    RenderReceiver m_receiver;
    RenderHandler m_handler;
    RenderSender m_sender;
  };

}
#endif
