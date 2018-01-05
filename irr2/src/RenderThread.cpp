#include "RenderThread.h"
#include "RenderLog.h"

using namespace irr;

RenderThread::RenderThread()
    : m_thread(nullptr) {
}

RenderThread::~RenderThread() {
  m_thread = nullptr;
}

void RenderThread::start() {
  m_thread = std::make_shared<boost::thread>(boost::bind(&RenderThread::thread_handler, this));
}

void RenderThread::join() {
  m_thread->join();
}

void RenderThread::thread_handler() {
  irr_log_info("enter");
  process();
}

void RenderThread::dump_data_raw(const char *data, const size_t length) {
  #if 0
  if (!data) {
    irr_log_err("data is null");
    return;
  }

  irr_log_info("dump %d bytes", length);
  const size_t row_length = 16;
  for (size_t r = 0; r < length / row_length; r++) {
    irr_log_plain("%8d: ", r);
    for (size_t c = 0; c < row_length; c++) {
      irr_log_plain("0x%02x ", 0xFF & data[r * row_length + c]);
    }
    irr_log_plain("\n");
  }
  for (size_t c = 0; c < length % row_length; c++) {
    irr_log_plain("0x%x ", data[(length / row_length) * row_length + c]);
  }
  irr_log_plain("\n");
  #endif
}

