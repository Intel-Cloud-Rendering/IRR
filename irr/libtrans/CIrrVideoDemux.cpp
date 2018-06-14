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

#include "CIrrVideoDemux.h"

CIrrVideoDemux::CIrrVideoDemux(int w, int h, int format, float framerate) {
    m_Info.m_pCodecPars->codec_type = AVMEDIA_TYPE_VIDEO;
    m_Info.m_pCodecPars->codec_id   = AV_CODEC_ID_RAWVIDEO;
    m_Info.m_pCodecPars->format     = format;
    m_Info.m_pCodecPars->width      = w;
    m_Info.m_pCodecPars->height     = h;
    m_Info.m_rFrameRate             = av_d2q(framerate, 1024);
    m_Info.m_rTimeBase              = AV_TIME_BASE_Q;
    m_nStartTime                    = av_gettime_relative();
    m_nNextPts                      = 0;
    av_new_packet(&m_Pkt,
                  av_image_get_buffer_size(AVPixelFormat(format), w, h, 32));
}

CIrrVideoDemux::~CIrrVideoDemux() {
    av_packet_unref(&m_Pkt);
}


int CIrrVideoDemux::getNumStreams() {
    return 1;
}

CStreamInfo* CIrrVideoDemux::getStreamInfo(int strIdx) {
    return &m_Info;
}

int CIrrVideoDemux::readPacket(AVPacket *avpkt) {
    int ret;

    while (av_gettime_relative() - m_nStartTime < m_nNextPts)
        av_usleep(1000);

    std::lock_guard<std::mutex> lock(m_Lock);

    m_Pkt.pts  = m_Pkt.dts = m_nNextPts;
    ret = av_packet_ref(avpkt, &m_Pkt);

    m_nNextPts += av_rescale_q(1, av_inv_q(m_Info.m_rFrameRate), m_Info.m_rTimeBase);

    return ret;
}

int CIrrVideoDemux::sendPacket(AVPacket *pkt) {
    std::lock_guard<std::mutex> lock(m_Lock);

    av_packet_unref(&m_Pkt);
    av_packet_move_ref(&m_Pkt, pkt);

    return 0;
}
