#ifndef RENDER_HANDLER_H
#define RENDER_HANDLER_H
#include <memory>
#include "RenderChannel.h"
#include "RenderThread.h"
#include "RenderThreadInfo.h"
#include "IOStream.h"
#include "ChecksumCalculatorThreadInfo.h"

namespace irr {

#define CMD_HEADER_BUF_SIZE 8

  class RenderHandler : public RenderThread {
 public:
    RenderHandler(RenderChannel& channel);
    RenderHandler(const RenderHandler&) = delete;
    RenderHandler(RenderHandler&&);
    RenderHandler& operator=(const RenderHandler&) = delete;
    RenderHandler& operator=(RenderHandler&&) = delete;
    ~RenderHandler();
    virtual void process();
    size_t do_decode(char *, size_t, RenderThreadInfo *,
                     IOStream *, ChecksumCalculator *);
    void terminate();
 private:
    RenderChannel& m_channel;
    std::shared_ptr<char> m_left_cmd_buf;
    char m_cmd_header_buf[CMD_HEADER_BUF_SIZE];
    size_t m_left_cmd_buf_sz;
    bool m_terminate;
    size_t m_cmd_count;
  };
}
#endif
