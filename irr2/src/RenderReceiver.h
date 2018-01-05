#ifndef RENDER_RECEIVER_H
#define RENDER_RECEIVER_H
#include <boost/asio.hpp>
#include "RenderChannel.h"

using boost::asio::ip::tcp;

namespace irr {

  typedef struct GLCmdPacketHead {
    int packet_type : 8;
    int packet_body_size : 24;
  }GLCmdPacketHead;

  const unsigned PACKET_HEAD_LEN = sizeof(GLCmdPacketHead);
  
  class RenderReceiver {
 public:
    RenderReceiver(tcp::socket&, RenderChannel&);
    RenderReceiver(const RenderReceiver&);
    RenderReceiver(RenderReceiver&&);
    RenderReceiver& operator=(const RenderReceiver&) = delete;
    RenderReceiver& operator=(RenderReceiver&&) = delete;
    ~RenderReceiver();
    void receive();
 private:
    tcp::socket& m_socket;
    RenderChannel& m_channel;
    InBuffer *m_body_buf;
    size_t m_body_buf_transferred;
    enum {
      max_length = 8192
    };
    char m_data[max_length];

    void on_read_head(const boost::system::error_code& ec,
                      size_t bytes_transferred);
    void on_read_body(const boost::system::error_code& ec,
                      size_t bytes_transferred);
    void dump_data_raw(const char *, const size_t);
  };
}

#endif
