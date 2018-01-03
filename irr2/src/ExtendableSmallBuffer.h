#ifndef EXTENDABLE_SMALL_BUFFER
#define EXTENDABLE_SMALL_BUFFER
#include "stddef.h"

namespace irr {
  const size_t CHUNK_SIZE_128 = 128;
  const size_t CHUNK_SIZE_256 = 256;
  const size_t CHUNK_SIZE_512 = 512;
  /* This buffer class has an fix-sized internal small buffer *
   * by default, if required buffer length exceeds internal *
   * buffer length, fallback to dynamic allocation *
   * this class makes possible for pre-allocation of memory pool *
   */
  template <size_t INTERNAL_BUFFER_SIZE>
  class ExtendableSmallBuffer {
 public:
    /* buffer length must be decided in constructor */
    ExtendableSmallBuffer(const size_t);
    ExtendableSmallBuffer(const char *, const size_t);
    ExtendableSmallBuffer(const ExtendableSmallBuffer&);
    ExtendableSmallBuffer(ExtendableSmallBuffer&&);
    ExtendableSmallBuffer& operator=(const ExtendableSmallBuffer&);
    ExtendableSmallBuffer& operator=(ExtendableSmallBuffer&&);
    ~ExtendableSmallBuffer();
    bool appendSubBuffer(const char *buffer, const size_t start, const size_t end);
    const size_t length () const { return m_data_length; }
    char* data () const { return m_data; }
    void setDataNull() { m_data = nullptr; } /* for move ctor and assignment */
 private:
    bool use_internal() { return m_data_length > INTERNAL_BUFFER_SIZE ? false : true; }
    char *m_data;
    size_t m_data_length;
    char m_internal_data[INTERNAL_BUFFER_SIZE];
  };
}

#endif
