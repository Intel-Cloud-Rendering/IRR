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

#include "CCallbackMux.h"


CCallbackMux::CCallbackMux(void* opaque, cbOpen pOpen, cbWrite pWrite, cbClose pClose)
: CTransLog(__func__), m_Opaque(opaque), m_pOpen(pOpen), m_pWrite(pWrite),
    m_pClose(pClose), m_bInited(false)
{ }

CCallbackMux::~CCallbackMux() {
    if (m_pClose)
        m_pClose(m_Opaque);
}

int CCallbackMux::addStream(int, CStreamInfo* info) {
    int ret = 0;
    ///< TODO: Deal with multi-channel streams.
    if (m_bInited || info->m_pCodecPars->codec_type != AVMEDIA_TYPE_VIDEO) {
        Warn("Do not support multi-streams or non-video streams so far.\n");
        return AVERROR(EINVAL);
    }

    if (m_pOpen) {
        ret = m_pOpen(m_Opaque, info->m_pCodecPars->width, info->m_pCodecPars->height,
                      av_q2d(info->m_rFrameRate));
        if (ret < 0) {
            Error("Fail to call cb->open().\n");
            return ret;
        }
    }

    m_bInited = true;

    return 0;
}

int CCallbackMux::write(AVPacket *pPkt) {
    int ret = 0;
    if (!pPkt)  ///< Do not support flush
        return 0;

    if (m_pWrite) {
        ret = m_pWrite(m_Opaque, pPkt->data, static_cast<size_t>(pPkt->size));
        if (ret < 0) {
            Error("Fail to write back by calling cb->write(). EC %d.\n", ret);
            return ret;
        }
    }

    return ret;
}
