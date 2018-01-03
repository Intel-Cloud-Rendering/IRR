#include <memory.h>
#include "RenderSession.h"
#include "RenderLog.h"

using namespace irr;

RenderSession::RenderSession(std::shared_ptr<tcp::socket> socket)
    : m_channel(std::make_shared<RenderChannel>()),
      m_receiver(socket, m_channel),
      m_handler(m_channel),
      m_sender(socket, m_channel) {
  m_handler.start();
  m_sender.start();
}

RenderSession::RenderSession(RenderSession&& other)
    : m_channel(std::move(other.m_channel)),
      m_receiver(other.m_receiver),
      m_handler(std::move(other.m_handler)),
      m_sender(std::move(other.m_sender)) {
  other.m_channel = nullptr;
}

RenderSession::~RenderSession() {
  irr_log_info("");
}



