#include "RenderDispatch.h"
#include "RenderServer.h"
#include "RenderLog.h"
#include "RendererImpl.h"
#include <memory>
#include <unistd.h>

using namespace irr;

RenderServer::RenderServer(boost::asio::io_service& io_service, short port)
    : AsioServer(io_service, port) {
}

RenderServer::~RenderServer() {
  irr_log_info("Deconstruct render server");
}

void RenderServer::handle_accept() {
  irr_log_info("create render session");
  sessions.emplace_back(m_socket);
}

void RenderServer::handle_terminate() {
  for (std::vector<RenderSession>::iterator it = sessions.begin();
       it != sessions.end(); it++) {
    it->terminate();
  }
  for (std::vector<RenderSession>::iterator it = sessions.begin();
       it != sessions.end(); it++) {
    it->join_all();
  }
}

void RenderServer::init() {
  if (!init_egl_dispatch()) {
    irr_log_err("failed to init egl dispatch");
    return;
  }
  if (!init_gles1_dispatch()) {
    irr_log_err("failed to init gles1 dispatch");
    return;
  }
  if (!init_gles2_dispatch()) {
    irr_log_err("failed to init gles2 dispatch");
    return;
  }
  
  std::shared_ptr<emugl::RendererImpl> render_impl = std::make_shared<emugl::RendererImpl>();
  if (!render_impl->initialize(1080, 1920, true)) {
    irr_log_err("failed to initialize renderer");
    return;
  }
  m_renderer = render_impl;
}
