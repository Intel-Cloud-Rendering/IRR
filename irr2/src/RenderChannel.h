#ifndef RENDER_CHANNEL_H
#define RENDER_CHANNEL_H
#include "BufferQueue.h"
#include "ExtendableSmallBuffer.h"
#include <boost/pool/singleton_pool.hpp>

namespace irr {

  using InBuffer = ExtendableSmallBuffer<CHUNK_SIZE_512>;
  using OutBuffer = ExtendableSmallBuffer<CHUNK_SIZE_128>;

  class RenderChannel {
 public:
    RenderChannel();
    RenderChannel(const RenderChannel&) = delete;
    RenderChannel(RenderChannel&&) = delete;
    RenderChannel& operator=(const RenderChannel&) = delete;
    RenderChannel& operator=(RenderChannel&&) = delete;
    ~RenderChannel();
    InBuffer *newInBuffer(const size_t);
    InBuffer *newInBuffer(const char *, const size_t);
    void deleteInBuffer(InBuffer *const);
    OutBuffer *newOutBuffer(const char *, const size_t);
    OutBuffer *newOutBuffer(const size_t);
    void deleteOutBuffer(OutBuffer *const);
    void writeIn(InBuffer *const in);
    bool tryReadIn(InBuffer *& in);
    void ReadIn(InBuffer *&in);
    void writeOut(OutBuffer *const out);
    bool tryReadOut(OutBuffer *& out);
    void ReadOut(OutBuffer *& out);

 private:
    BufferQueue<InBuffer *> inBufferQueue;
    BufferQueue<OutBuffer *> outBufferQueue;
    struct InBufferPoolTag {};
    struct OutBufferPoolTag {};
    using inBufferPool = boost::singleton_pool<InBufferPoolTag, sizeof(InBuffer)>;
    using outBufferPool = boost::singleton_pool<OutBufferPoolTag, sizeof(OutBuffer)>;
  };

}
#endif
