#ifndef RENDER_THREAD_H
#define RENDER_THREAD_H
#include <boost/thread.hpp>
#include <memory>

namespace irr {

  class RenderThread {
 public:
    RenderThread();
    ~RenderThread();
    void start();
    virtual void process() = 0;
    void join();
 private:
    std::shared_ptr<boost::thread> m_thread;
    void thread_handler();
 protected:
    /* convient member function, should't be here */
    void dump_data_raw(const char *data, const size_t length);
  };
}
#endif
