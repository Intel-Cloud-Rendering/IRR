#ifndef RENDER_SESSION_H
#define RENDER_SESSION_H
#include <boost/asio.hpp>
#include <memory>
#include "AsioConnection.h"
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

  class RenderSession : public AsioConnection {
 public:
    RenderSession();
    RenderSession(const RenderSession&) = delete;
    RenderSession(RenderSession&&);
    RenderSession& operator=(const RenderSession&) = delete;
    RenderSession& operator=(RenderSession&&) = delete;
    ~RenderSession();
    virtual void handle_terminate();
    virtual void handle_process();

 private:
    RenderChannel m_channel;
    RenderReceiver m_receiver;
    RenderHandler m_handler;
    RenderSender m_sender;
  };

}
#endif
