#include "RenderHandler.h"
#include "RenderLog.h"
#include "FrameBuffer.h"
#include "RenderControl.h"
#include "OpenGLESDispatch/EGLDispatch.h"
#include "OpenGLESDispatch/GLESv2Dispatch.h"
#include "OpenGLESDispatch/GLESv1Dispatch.h"

using namespace irr;

RenderHandler::RenderHandler(std::shared_ptr<RenderChannel> channel)
    : m_channel(channel),
      m_terminate(false),
      m_left_cmd_buf(nullptr),
      m_left_cmd_buf_sz(0),
      m_cmd_count(0) {
  
}

RenderHandler::RenderHandler(RenderHandler&& other)
    : m_channel(std::move(other.m_channel)),
      m_terminate(other.m_terminate),
      m_left_cmd_buf(other.m_left_cmd_buf),
      m_left_cmd_buf_sz(other.m_left_cmd_buf_sz),
      m_cmd_count(other.m_cmd_count) {
  other.m_channel = nullptr;
  other.m_left_cmd_buf = nullptr;
}

RenderHandler::~RenderHandler() {
}

void RenderHandler::terminate() {
  m_terminate = true;
  /* create a dummy in buffer *
   * to awake process */
  InBuffer *in = m_channel->newInBuffer(1);
  m_channel->writeIn(in);
}

size_t RenderHandler::do_decode(char *data, size_t total, RenderThreadInfo *info,
                              IOStream *io_stream, ChecksumCalculator *checksum) {
  char *buf = data;
  size_t left = total;
  size_t last = 0;
  while (left > 0) {
    size_t cmdSz = *(const int32_t*)(buf + 4);
    m_cmd_count++;
    irr_log_info("left = %d, command size = %d, command count = %d", left, cmdSz, m_cmd_count);

    /* Buffer does not contain all data *
     * keep the remaining command data *
     * in left cmd buffer and return *
     */
    if (left < cmdSz) {
      irr_assert(m_left_cmd_buf == nullptr);
      m_left_cmd_buf = std::shared_ptr<char>(new char[cmdSz], std::default_delete<char[]>());
      std::memcpy(m_left_cmd_buf.get(), buf, left);
      m_left_cmd_buf_sz = cmdSz;
      return left;
    }
    
    FrameBuffer::getFB()->lockContextStructureRead();
    last = info->m_glDec.decode(buf, cmdSz,
                                io_stream, checksum);
    irr_assert(last == 0 || last == cmdSz);
          
    last = info->m_gl2Dec.decode(buf, cmdSz,
                                 io_stream, checksum);
    FrameBuffer::getFB()->unlockContextStructureRead();
    irr_assert(last == 0 || last == cmdSz);
        
    last = info->m_rcDec.decode(buf, cmdSz,
                                io_stream, checksum);
    irr_assert(last == 0 || last == cmdSz);
    
    buf += cmdSz;
    left -= cmdSz;
  }
  return 0;
}

void RenderHandler::process() {
  irr_log_info("enter");
  /* should be alloc locally in the thread */
  IOStream io_stream(m_channel);
  RenderThreadInfo tInfo;
  ChecksumCalculatorThreadInfo tChecksumInfo;
  ChecksumCalculator& checksumCalc = tChecksumInfo.get();

  tInfo.m_glDec.initGL(gles1_dispatch_get_proc_func, NULL);
  tInfo.m_gl2Dec.initGL(gles2_dispatch_get_proc_func, NULL);
  initRenderControlContext(&tInfo.m_rcDec);

  InBuffer *in;
  m_channel->ReadIn(in);
  irr_assert(in->length() == sizeof(uint32_t));
  dump_data_raw(in->data(), in->length());
  m_channel->deleteInBuffer(in);
  in = nullptr;

  size_t left = 0;
  while (!m_terminate) {
    m_channel->ReadIn(in);
    dump_data_raw(in->data(), in->length());

    /* some data left from last command processing */
    if (left) {
      /* not enough data to fill next command */
      if (left + in->length() < m_left_cmd_buf_sz) {
        std::memcpy(m_left_cmd_buf.get() + left, in->data(), in->length());
        left += in->length();
      }
      /* more data than next command need */
      else if (left + in->length() > m_left_cmd_buf_sz) {
        size_t remain_sz = m_left_cmd_buf_sz - left;
        std::memcpy(m_left_cmd_buf.get() + left, in->data(), remain_sz);
        left = do_decode(m_left_cmd_buf.get(), m_left_cmd_buf_sz, &tInfo, &io_stream, &checksumCalc);
        irr_assert(left == 0);
        m_left_cmd_buf = nullptr;
        m_left_cmd_buf_sz = 0;
        left = do_decode(in->data() + remain_sz, in->length() - remain_sz, &tInfo, &io_stream, &checksumCalc);
      }
      /* exactally fill next command */
      else {
        std::memcpy(m_left_cmd_buf.get() + left, in->data(), in->length());
        left = do_decode(m_left_cmd_buf.get(), m_left_cmd_buf_sz, &tInfo, &io_stream, &checksumCalc);
        irr_assert(left == 0);
        m_left_cmd_buf = nullptr;
        m_left_cmd_buf_sz = 0;
      }
    } /* !left */
    else {
      left = do_decode(in->data(), in->length(), &tInfo, &io_stream, &checksumCalc);
    }

    m_channel->deleteInBuffer(in);
    in = nullptr;
  }

  FrameBuffer::getFB()->bindContext(0, 0, 0);
  if (tInfo.currContext || tInfo.currDrawSurf || tInfo.currReadSurf) {
    irr_log_err("ERROR: RenderThread exiting with current context/surfaces\n");
  }

  FrameBuffer::getFB()->drainWindowSurface();
  FrameBuffer::getFB()->drainRenderContext();
  irr_log_info("finish");
}
