#include "RenderDispatch.h"
#include "RenderServer.h"
#include "RenderLog.h"
#include "RendererImpl.h"
#include <memory>
#include <unistd.h>
#ifdef DUMP_RAW_VIDEO
#include <fstream>
#endif
#ifdef PRINT_STAT
#include <boost/timer/timer.hpp>
using boost::timer::cpu_timer;
using boost::timer::cpu_times;
using boost::timer::nanosecond_type;
#endif

using namespace irr;

RenderServer::RenderServer(boost::asio::io_service& io_service, short port)
    : AsioServer(io_service, port) {
}

RenderServer::~RenderServer() {
  irr_log_info("Deconstruct render server");
}

std::shared_ptr<AsioConnection> RenderServer::create_connection() {
  std::shared_ptr<RenderSession> session = std::make_shared<RenderSession>();
  return std::static_pointer_cast<AsioConnection>(session);
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
  render_impl->setPostCallback(on_post_callback, nullptr);
  m_renderer = render_impl;
}


void RenderServer::on_post_callback(void *context, int width, int height,
                                    int ydir, int format, int type,
                                    unsigned char*pixels) {
#ifdef PRINT_STAT
  static cpu_timer timer;
  static size_t count = 1;
  cpu_times const elapsed_times(timer.elapsed());
  nanosecond_type const elapsed(elapsed_times.system + elapsed_times.user);
  if (elapsed) {
    unsigned long millisec = elapsed / 1000000;
    irr_log_stat("%ld fps", count * 1000 / millisec);
  }
  count ++;
#endif
#ifdef DUMP_RAW_VIDEO
  dump_to_files(width, height, ydir, format, type, pixels);
#endif
}

#ifdef DUMP_RAW_VIDEO
void RenderServer::dump_to_files(int width, int height,
                                 int ydir, int format, int type,
                                 unsigned char*pixels) {
  static std::ofstream dump_file("./dump/video.argb", std::ios::out);
  dump_file.write((const char *)pixels, width * height * 4);
}
#endif
