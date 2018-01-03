#ifndef RENDER_RECEIVER_H
#define RENDER_RECEIVER_H
#include <boost/asio.hpp>
#include <memory>
#include "RenderChannel.h"

using boost::asio::ip::tcp;

namespace irr {

  typedef struct GLCmdPacketHead {
    int packet_type : 8;
    int packet_body_size : 24;
  }GLCmdPacketHead;

  const unsigned PACKET_HEAD_LEN = sizeof(GLCmdPacketHead);
  
  class RenderReceiver{
 public:
    RenderReceiver(std::shared_ptr<tcp::socket> socket,
                   std::shared_ptr<RenderChannel> channel);
    RenderReceiver(const RenderReceiver&);
    RenderReceiver(RenderReceiver&&);
    RenderReceiver& operator=(const RenderReceiver&) = delete;
    RenderReceiver& operator=(RenderReceiver&&) = delete;
    ~RenderReceiver();

 private:
    std::shared_ptr<tcp::socket> m_socket;
    std::shared_ptr<RenderChannel> m_channel;
    enum {
      max_length = 8192
    };
    char m_data[max_length];
    void receive();
    void on_read_head(const boost::system::error_code& ec,
                      size_t bytes_transferred);
    void on_read_body(const boost::system::error_code& ec,
                      size_t bytes_transferred);
    void dump_data_raw(const char *, const size_t);
  };
}

#endif
