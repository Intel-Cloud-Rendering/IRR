#ifndef RENDER_SERVER_H
#define RENDER_SERVER_H
#include "AsioServer.h"
#include "RenderSession.h"
#include "Renderer.h"

#define DUMP_TO_FILE 1

namespace irr {

  class RenderServer : public AsioServer {
 public:
    RenderServer(boost::asio::io_service& io_service, short port);
    ~RenderServer();
    void init();
 protected:
    virtual std::shared_ptr<AsioConnection> create_connection();
 private:
#ifdef DUMP_TO_FILE
    static void dump_to_files(int, int, int, int, int, unsigned char*);
    static void on_post_callback(void *, int, int, int, int, int, unsigned char*);
#endif
    emugl::RendererPtr m_renderer;
  };
}

#endif
