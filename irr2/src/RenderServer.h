#ifndef RENDER_SERVER_H
#define RENDER_SERVER_H
#include "AsioServer.h"
#include "RenderSession.h"
#include "Renderer.h"

namespace irr {

  class RenderServer : public AsioServer {
 public:
    RenderServer(boost::asio::io_service& io_service, short port);
    ~RenderServer();
    void init();
 private:
    emugl::RendererPtr m_renderer;
    std::vector<RenderSession> sessions;
    virtual void handle_accept();
    virtual void handle_terminate();
  };
}

#endif
