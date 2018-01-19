#include "RenderReceiver.h"
#include "RenderLog.h"
#include "boost/bind.hpp"

using namespace irr;

RenderReceiver::RenderReceiver(tcp::socket& socket, RenderChannel& channel, bool& termed)
    : m_socket(socket),
      m_channel(channel),
      m_is_terminated(termed),
      m_body_buf(nullptr),
      m_body_buf_transferred(0) {
}

RenderReceiver::RenderReceiver(const RenderReceiver& other)
    :m_socket(other.m_socket),
     m_channel(other.m_channel),
     m_is_terminated(other.m_is_terminated) {
  std::memcpy(m_data, other.m_data, max_length);
}

RenderReceiver::RenderReceiver(RenderReceiver&& other)
    : m_socket(other.m_socket),
      m_channel(other.m_channel),
      m_is_terminated(other.m_is_terminated) {
  std::memcpy(m_data, other.m_data, max_length);
}

RenderReceiver::~RenderReceiver() {
  irr_log_info("");
}

void RenderReceiver::receive() {
  boost::asio::async_read(m_socket, boost::asio::buffer(m_data, PACKET_HEAD_LEN),
                          boost::bind(&RenderReceiver::on_read_head, this,
                                      boost::asio::placeholders::error,
                                      boost::asio::placeholders::bytes_transferred));
}


void RenderReceiver::on_read_head(const boost::system::error_code& ec,
                            size_t bytes_transferred) {
  if (!ec) {
    GLCmdPacketHead *head = (GLCmdPacketHead *)m_data;
    irr_log_info("type = %d, size = %d", head->packet_type, head->packet_body_size);
    m_body_buf = m_channel.newInBuffer(head->packet_body_size);
    m_body_buf_transferred = 0;
    size_t to_transfer = head->packet_body_size > max_length ? max_length : head->packet_body_size;
    if (to_transfer) {
      boost::asio::async_read(m_socket, boost::asio::buffer(m_data, to_transfer),
                              boost::bind(&RenderReceiver::on_read_body, this,
                                          boost::asio::placeholders::error,
                                          boost::asio::placeholders::bytes_transferred));
    }
  } else {
    irr_log_info("read closed");
    m_is_terminated = true;
  }
}

void RenderReceiver::on_read_body(const boost::system::error_code& ec,
                            size_t bytes_transferred) {
  if (!ec) {
    irr_log_info("Read %d bytes", bytes_transferred);
    irr_assert(m_body_buf != nullptr);
    std::memcpy(m_body_buf->data() + m_body_buf_transferred, m_data, bytes_transferred);
    m_body_buf_transferred += bytes_transferred;

    /* body all transferred */
    if (m_body_buf_transferred == m_body_buf->length()) {
      m_channel.writeIn(m_body_buf);
      m_body_buf = nullptr;
      m_body_buf_transferred = 0;
      boost::asio::async_read(m_socket, boost::asio::buffer(m_data, PACKET_HEAD_LEN),
                              boost::bind(&RenderReceiver::on_read_head, this,
                                          boost::asio::placeholders::error,
                                          boost::asio::placeholders::bytes_transferred));
    } /* body not yet transferred */
    else if (m_body_buf_transferred < m_body_buf->length()) {
      size_t not_transferred = m_body_buf->length() - m_body_buf_transferred;
      size_t to_transfer = not_transferred > max_length ? max_length : not_transferred;
      irr_log_info("not yet transferred %d bytes to transfer", to_transfer);
      boost::asio::async_read(m_socket, boost::asio::buffer(m_data, to_transfer),
                              boost::bind(&RenderReceiver::on_read_body, this,
                                          boost::asio::placeholders::error,
                                          boost::asio::placeholders::bytes_transferred));
    } /* body over transferred */
    else {
      irr_assert(0);
    }

  } else {
    irr_log_info("read closed");
    m_is_terminated = true;
  }
}

void RenderReceiver::dump_data_raw(const char *data, const size_t length) {
  if (!data) {
    irr_log_err("data is null");
    return;
  }

  irr_log_info("dump %d bytes", length);
  const size_t row_length = 16;
  for (size_t r = 0; r < length / row_length; r++) {
    for (size_t c = 0; c < row_length; c++) {
      irr_log_plain("0x%x ", data[r * row_length + c]);
    }
    irr_log_plain("\n");
  }
  for (size_t c = 0; c < length % row_length; c++) {
    irr_log_plain("0x%x ", data[(length / row_length) * row_length + c]);
  }
  irr_log_plain("\n");
}


/*
void RenderReceiver::on_read(const boost::system::error_code& ec,
                            size_t bytes_transferred) {
  if (!ec) {
    dump_data_raw(m_data, bytes_transferred);
    irr_log_info("m_body_to_read = %d", m_body_to_read);
    if (m_body_to_read == 0) {
      read_head(m_data, bytes_transferred);
    } else {
      read_body(m_data, bytes_transferred);
    }

    m_socket->async_read_some(boost::asio::buffer(m_data, max_length),
                              boost::bind(&RenderReceiver::on_read, this,
                                          boost::asio::placeholders::error,
                                          boost::asio::placeholders::bytes_transferred));
  }
}

void RenderReceiver::read_head(const char *buf, const size_t len) {
  if (!buf || !len)
    return;
  irr_assert(len >= PACKET_HEAD_LEN);
  GLCmdPacketHead * head = (GLCmdPacketHead *)buf;
  irr_log_info(" type = %d, size = %d", head->packet_type, head->packet_body_size);
  m_body_to_read = head->packet_body_size;
  if (!m_body_to_read)
    return;
  irr_assert(m_in == nullptr);
  m_in = m_channel->newInBuffer(head->packet_body_size);
  read_body(buf + PACKET_HEAD_LEN, len - PACKET_HEAD_LEN);
}

void RenderReceiver::read_body(const char *buf, const size_t len) {
  if (!buf || !len)
    return;
  irr_assert(m_in != nullptr);
  size_t to_append = m_body_to_read > len ? len : m_body_to_read;
  size_t start = m_in->length() - m_body_to_read;
  size_t end = start + to_append;
  m_in->appendSubBuffer(buf, start, end);
  m_body_to_read -= to_append;
  if (!m_body_to_read) {
    m_channel->writeIn(m_in);
    m_in = nullptr;
  }
  read_head(buf + to_append, len - to_append);
}
*/
