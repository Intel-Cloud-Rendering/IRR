#ifndef RENDER_SERVER_H
#define RENDER_SERVER_H
#include "AsioServer.h"
#include "RenderSession.h"
#include "Renderer.h"

//#define DUMP_RAW_VIDEO 1
#define PRINT_STAT 1

namespace irr {

  class RenderServer : public AsioServer {
 public:
    RenderServer(boost::asio::io_service& io_service, short port);
    ~RenderServer();
    void init();
 protected:
    virtual std::shared_ptr<AsioConnection> create_connection();
 private:
#ifdef DUMP_RAW_VIDEO
    static void dump_to_files(int, int, int, int, int, unsigned char*);
#endif
    static void on_post_callback(void *, int, int, int, int, int, unsigned char*);
    emugl::RendererPtr m_renderer;
  };
}

#endif
