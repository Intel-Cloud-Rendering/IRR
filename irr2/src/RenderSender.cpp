#include "RenderSender.h"
#include "RenderLog.h"
#include <boost/bind.hpp>

using namespace irr;

RenderSender::RenderSender(tcp::socket& socket, RenderChannel& channel)
    : m_socket(socket),
      m_channel(channel),
      m_terminate(false) {
}

RenderSender::RenderSender(RenderSender&& other)
    : m_channel(other.m_channel),
      m_socket(other.m_socket),
      m_terminate(other.m_terminate) {
  other.m_terminate = false;
}
RenderSender::~RenderSender() {
}

void RenderSender::terminate() {
  m_terminate = true;
  /* create a dummy out buffer *
   * to awake process */
  OutBuffer *out = m_channel.newOutBuffer(1);
  m_channel.writeOut(out);
}

void RenderSender::process() {
  irr_log_info("RenderSender enter process");
  while(!m_terminate) {
    OutBuffer *out;
    m_channel.ReadOut(out);
    irr_log_info("RenderSender send %d bytes", out->length());
    dump_data_raw(out->data(), out->length());
    boost::asio::write(m_socket, boost::asio::buffer(out->data(), out->length()));
    m_channel.deleteOutBuffer(out);
  }
  irr_log_info("RenderSender exit process");
}
