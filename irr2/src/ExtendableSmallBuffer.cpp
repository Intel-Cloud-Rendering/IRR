#include "ExtendableSmallBuffer.h"
#include <cstring>
#include <RenderLog.h>

using namespace irr;

template <size_t INTERNAL_BUFFER_SIZE>
ExtendableSmallBuffer<INTERNAL_BUFFER_SIZE>::ExtendableSmallBuffer(const size_t length)
    : m_data(m_internal_data), m_data_length(length) {
  if (length == 0) {
    irr_assert(0);
    return;
  }
  if (!use_internal()) {
    m_data = new char[length];
  }
}

template <size_t INTERNAL_BUFFER_SIZE>
ExtendableSmallBuffer<INTERNAL_BUFFER_SIZE>::ExtendableSmallBuffer(const char *buffer, const size_t length)
    : m_data(m_internal_data), m_data_length(length) {
  if (!buffer || length == 0) {
    irr_assert(0);
    return;
  }
  if (!use_internal()) {
    m_data = new char[length];
  }
  std::memcpy(m_data, buffer, m_data_length);
}

template <size_t INTERNAL_BUFFER_SIZE>
ExtendableSmallBuffer<INTERNAL_BUFFER_SIZE>::ExtendableSmallBuffer(const ExtendableSmallBuffer<INTERNAL_BUFFER_SIZE>& other)
    : m_data(m_internal_data), m_data_length(other.length()) {
  if (!use_internal()) {
    m_data = new char[m_data_length];
  }
  std::memcpy(m_data, other.data(), m_data_length);
}

template <size_t INTERNAL_BUFFER_SIZE>
ExtendableSmallBuffer<INTERNAL_BUFFER_SIZE>::ExtendableSmallBuffer(ExtendableSmallBuffer<INTERNAL_BUFFER_SIZE>&& other)
    : m_data(other.data()), m_data_length(other.length()) {
  other.setDataNull();
}

template <size_t INTERNAL_BUFFER_SIZE>
ExtendableSmallBuffer<INTERNAL_BUFFER_SIZE>& ExtendableSmallBuffer<INTERNAL_BUFFER_SIZE>::operator=(const ExtendableSmallBuffer<INTERNAL_BUFFER_SIZE>& other) {
  if (!use_internal() && m_data) {
    delete[] m_data;
  }
  m_data_length = other.length();
  m_data = m_internal_data;
  if (!use_internal()) {
    m_data = new char[m_data_length];
  }
  std::memcpy(m_data, other.data(), m_data_length);
  return *this;
}

template <size_t INTERNAL_BUFFER_SIZE>
ExtendableSmallBuffer<INTERNAL_BUFFER_SIZE>& ExtendableSmallBuffer<INTERNAL_BUFFER_SIZE>::operator=(ExtendableSmallBuffer<INTERNAL_BUFFER_SIZE>&& other) {
  if (!use_internal() && m_data) {
    delete[] m_data;
  }
  m_data = other.data();
  m_data_length = other.length();
  other.setDataNull();
  return *this;
}

template <size_t INTERNAL_BUFFER_SIZE>
bool ExtendableSmallBuffer<INTERNAL_BUFFER_SIZE>::appendSubBuffer(const char *buffer, const size_t start, const size_t end) {
  if (!buffer || start >= end || end > m_data_length) {
    irr_assert(0);
    return false;
  }
  std::memcpy(m_data + start, buffer, end - start);
  return true;
}

template <size_t INTERNAL_BUFFER_SIZE>
ExtendableSmallBuffer<INTERNAL_BUFFER_SIZE>::~ExtendableSmallBuffer() {
  if (!use_internal() && m_data) {
    delete[] m_data;
  }
  m_data = nullptr;
  m_data_length = 0;
}

template class ExtendableSmallBuffer<CHUNK_SIZE_512>;
template class ExtendableSmallBuffer<CHUNK_SIZE_128>;
