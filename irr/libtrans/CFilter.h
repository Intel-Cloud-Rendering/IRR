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

#ifndef CFILTER_H
#define CFILTER_H

extern "C" {
#include <libavfilter/avfilter.h>
}

#include "CStreamInfo.h"

class CFilter {
public:
    CFilter() {}
    virtual ~CFilter() {}
    virtual int push(AVFrame *frame) { return 0; }
    virtual AVFrame* pop(void) { return nullptr; }
    virtual int getNumFrames(void) { return 0; }
    virtual CStreamInfo *getSinkInfo() { return nullptr; }
    virtual CStreamInfo *getSrcInfo() { return nullptr; }
};

#endif /* CFILTER_H */

