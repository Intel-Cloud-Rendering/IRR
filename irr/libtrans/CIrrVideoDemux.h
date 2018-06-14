/*
 * Copyright (C) 2018 Intel
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

#ifndef CIRRVIDEODEMUX_H
#define CIRRVIDEODEMUX_H

#include <mutex>
extern "C" {
#include <libavutil/time.h>
#include <libavutil/imgutils.h>
}
#include "CDemux.h"

class CIrrVideoDemux : public CDemux {
public:
    CIrrVideoDemux(int w, int h, int format, float framerate);
    ~CIrrVideoDemux();

    int getNumStreams();
    CStreamInfo* getStreamInfo(int strIdx);
    int readPacket(AVPacket *avpkt);
    int sendPacket(AVPacket *pkt);

private:
    std::mutex                  m_Lock;
    CStreamInfo                 m_Info;
    AVPacket                    m_Pkt;
    int64_t                     m_nStartTime;
    int64_t                     m_nNextPts;
};

#endif /* CIRRVIDEODEMUX_H */

