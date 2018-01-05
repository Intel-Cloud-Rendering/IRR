/*
* Copyright (C) 2011 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
#pragma once

#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory>
#include "RenderChannel.h"
#include "RenderLog.h"

/* Modified IOStream to coupe with *
 * Render Channel and Decoders *
 */
class IOStream {
public:
  explicit IOStream(irr::RenderChannel& channel)
    : m_channel(channel),
      m_out(nullptr) {
  }

  ~IOStream() {}

  unsigned char* alloc(size_t len) {
    irr_assert(m_out == nullptr);
    m_out = m_channel.newOutBuffer(len);
    return (unsigned char *)m_out->data();
  }

  int flush() {
    int len = 0;
    irr_assert(m_out != nullptr);
    len = m_out->length();
    m_channel.writeOut(m_out);
    m_out = nullptr;
    return len;
  }

  void setId(int id) {
    m_id = id;
  }

  int getId() {
    return m_id;
  }

  void* getDmaForReading(uint64_t guest_paddr) {
    irr_assert(0);
    return nullptr;
  }

  void unlockDma(uint64_t guest_paddr) {
    irr_assert(0);
  }

private:
  int m_id;
  irr::OutBuffer *m_out;
  irr::RenderChannel& m_channel;
};
