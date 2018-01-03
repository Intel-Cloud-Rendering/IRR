#ifndef BUFFER_QUEUE_H
#define BUFFER_QUEUE_H
#include <queue>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>

namespace irr {

  template <typename T>
  class BufferQueue {
 private:
    std::queue<T> m_que;
    mutable boost::mutex m_mtx;
    boost::condition_variable m_cond;
 public:
    void push(const T& buf) {
      boost::mutex::scoped_lock lock(m_mtx);
      m_que.push(buf);
      lock.unlock();
      m_cond.notify_one();
    }

    bool empty() const {
      boost::mutex::scoped_lock lock(m_mtx);
      return m_que.empty();
    }

    bool try_pop(T& buf) {
      boost::mutex::scoped_lock lock(m_mtx);
      if (m_que.empty()) {
        return false;
      }
      buf = m_que.front();
      m_que.pop();
      return true;
    }

    void wait_and_pop(T& buf) {
      boost::mutex::scoped_lock lock(m_mtx);
      while (m_que.empty()) {
        m_cond.wait(lock);
      }
      buf = m_que.front();
      m_que.pop();
    }
  };
    
}
#endif
