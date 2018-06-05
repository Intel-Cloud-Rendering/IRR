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

#ifndef CFFDEMUX_H
#define CFFDEMUX_H

extern "C" {
#include <libavformat/avformat.h>
}
#include "CTransLog.h"
#include "CDemux.h"

/**
 * FFmpeg demuxers
 */
class CFFDemux : private CTransLog, public CDemux {
public:
    CFFDemux(const char *url);
    ~CFFDemux();
    int start(const char *format = nullptr, AVDictionary *pDict = nullptr);
    int getNumStreams();
    CStreamInfo* getStreamInfo(int strIdx);
    int readPacket(AVPacket *avpkt);
    int seek(long offset, int whence);
    int tell();
    int rewind();
    int size();

private:
    CFFDemux() = delete;
    CFFDemux(const CFFDemux& orig) = delete;
    int init(const char *url, const char *format, AVDictionary *pDict);

private:
    AVFormatContext *m_pDemux;
    std::map<int, CStreamInfo*> m_mStreamInfos;
    std::string m_sUrl;
};

#endif /* CFFDEMUX_H */

