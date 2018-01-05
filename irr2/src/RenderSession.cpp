#include <memory.h>
#include "RenderSession.h"
#include "RenderLog.h"

using namespace irr;

RenderSession::RenderSession()
    : m_channel(),
      m_receiver(m_socket, m_channel),
      m_handler(m_channel),
      m_sender(m_socket, m_channel) {
}

RenderSession::RenderSession(RenderSession&& other)
    : m_channel(other.m_channel),
      m_receiver(other.m_receiver),
      m_handler(std::move(other.m_handler)),
      m_sender(std::move(other.m_sender)) {
}

void RenderSession::handle_terminate() {
  m_handler.terminate();
  m_sender.terminate();
  m_handler.join();
  m_sender.join();
}

void RenderSession::handle_process() {
  m_sender.start();
  m_handler.start();
  m_receiver.receive();
}

RenderSession::~RenderSession() {
  irr_log_info("");
}



