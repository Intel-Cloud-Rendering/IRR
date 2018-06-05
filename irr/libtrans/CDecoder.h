/*
 * Copyright (C) 2017 Intel
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef CDECODER_H
#define CDECODER_H

#include <queue>
extern "C" {
#include <libavcodec/avcodec.h>
}
#include "CStreamInfo.h"

class CDecoder {
public:
    CDecoder() {}
    virtual ~CDecoder() {}

    /**
     * 1. NULL to flush
     * 2. 0 on success, negative on failure
     */
    virtual int write(AVPacket *avpkt) { return AVERROR(EINVAL); }
    virtual int write(uint8_t *data, int size) { return AVERROR(EINVAL); }

    virtual AVFrame* read(void) { return nullptr; }

    ///< Get the number of frames decoded.
    virtual int getNumFrames(void) { return 0; }
    virtual CStreamInfo* getDecInfo() { return nullptr; }
};

#endif /* CDECODER_H */
