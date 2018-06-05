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

#ifndef CENCODER_H
#define CENCODER_H

extern "C" {
#include <libavutil/frame.h>
#include <libavcodec/avcodec.h>
}

#include "CStreamInfo.h"

class CEncoder {
public:
    CEncoder() {}
    virtual ~CEncoder() {}
    virtual int write(AVFrame *frame) { return 0; }
    virtual int read(AVPacket *pkt) { return 0; }
    virtual CStreamInfo *getStreamInfo() { return 0; }
};

#endif /* CENCODER_H */

