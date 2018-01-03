#include "RenderChannel.h"

using namespace irr;

RenderChannel::RenderChannel() {
  
}

RenderChannel::~RenderChannel() {
  inBufferPool::purge_memory();
  outBufferPool::purge_memory();
}

InBuffer *RenderChannel::newInBuffer(const size_t len) {
  void *buf = inBufferPool::malloc();
  InBuffer *in = new (buf) InBuffer(len);
  return in;
}

InBuffer *RenderChannel::newInBuffer(const char * data, const size_t len) {
  void *buf = inBufferPool::malloc();
  InBuffer *in = new (buf) InBuffer(data, len);
  return in;
}

void RenderChannel::deleteInBuffer(InBuffer *const in) {
  in->~InBuffer();
  inBufferPool::free(in);
}

OutBuffer *RenderChannel::newOutBuffer(const size_t len) {
  void *buf = outBufferPool::malloc();
  OutBuffer *out = new (buf) OutBuffer(len);
  return out;
}

OutBuffer *RenderChannel::newOutBuffer(const char *data, const size_t len) {
  void *buf = outBufferPool::malloc();
  OutBuffer *out = new (buf) OutBuffer(data, len);
  return out;
}

void RenderChannel::deleteOutBuffer(OutBuffer *const out) {
  out->~OutBuffer();
  outBufferPool::free(out);
}

void RenderChannel::writeIn(InBuffer *const in) {
  inBufferQueue.push(in);
}

bool RenderChannel::tryReadIn(InBuffer *& in) {
  return inBufferQueue.try_pop(in);
}

void RenderChannel::ReadIn(InBuffer *& in) {
  inBufferQueue.wait_and_pop(in);
}

void RenderChannel::writeOut(OutBuffer *const out) {
  outBufferQueue.push(out);
}

bool RenderChannel::tryReadOut(OutBuffer *& out) {
  return outBufferQueue.try_pop(out);
}

void RenderChannel::ReadOut(OutBuffer *& out) {
  outBufferQueue.wait_and_pop(out);
}
