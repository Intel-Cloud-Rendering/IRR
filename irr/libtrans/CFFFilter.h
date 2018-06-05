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

#ifndef CFFFILTER_H
#define CFFFILTER_H

#include "CFilter.h"
#include "CStreamInfo.h"
#include "CTransLog.h"
extern "C" {
#include <libavfilter/avfilter.h>
//#include <libavfilter/avfiltergraph.h>
#include "libavutil/log.h"
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
}

class CFFFilter : public CFilter, private CTransLog {
public:
    CFFFilter(CStreamInfo *in, CStreamInfo *out);
    ~CFFFilter();
    int push(AVFrame *frame);
    AVFrame* pop();
    int getNumFrames(void);
    CStreamInfo *getSinkInfo();
    CStreamInfo *getSrcInfo();

private:
    AVFilterGraph *m_pGraph;
    bool           m_bInited;
    AVFilterContext *m_pSrc, *m_pSink;
    size_t         m_nFrames;
    CStreamInfo    m_SrcInfo, m_SinkInfo;

private:
    AVFilterContext* alloc_filter(const char *name, const char *par);
};

#endif /* CFFFILTER_H */

