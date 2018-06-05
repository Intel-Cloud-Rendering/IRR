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

#include <map>
#include <bits/basic_string.h>
#include "CFFMux.h"

using namespace std;

CFFMux::CFFMux(const char *url, const char *format) : CTransLog(__func__) {
    int ret;
    AVOutputFormat *oFmt = av_guess_format(format, url, nullptr);

    m_sUrl = url;
    m_bInited = false;
    m_pFmt = nullptr;

    if (!oFmt) {
        oFmt = av_guess_format("mpegts", url, nullptr);
        if (!oFmt) {
            Error("Fail to find a suit Muxer for %s.\n", url);
            return;
        }
    }

    ret = avformat_alloc_output_context2(&m_pFmt, oFmt, nullptr, url);
    if (!m_pFmt || ret != 0) {
        Error("Fail to alloc Mux context.Msg: %s\n", ErrToStr(ret).c_str());
        return;
    }

    if (!(oFmt->flags & AVFMT_NOFILE)) {
        ret = avio_open(&m_pFmt->pb, url, AVIO_FLAG_WRITE);
        if (ret != 0) {
            avformat_free_context(m_pFmt);
            Error("Fail to open corresponding file. Error %s\n", ErrToStr(ret).c_str());
            return;
        }
    }
}

CFFMux::~CFFMux() {
    if (m_pFmt) {
        for (unsigned id = 0; id < m_pFmt->nb_streams; id++)
            Info("Total written frames on stream[%d]: %d.\n",
                 id, m_pFmt->streams[id]->nb_frames);
        av_write_trailer(m_pFmt);
        if (m_pFmt->pb)
            avio_close(m_pFmt->pb);
        avformat_free_context(m_pFmt);
    }
}

int CFFMux::addStream(int idx, CStreamInfo *info) {
    int ret = 0;
    AVStream *st;

    if (!m_pFmt || m_mIdxs.find(idx) != m_mIdxs.end())
        return AVERROR(EINVAL);

    st = avformat_new_stream(m_pFmt, nullptr);
    if (!st) {
        Error("Fail to alloc stream context.\n");
        return AVERROR(ENOMEM);
    }

    ret = avcodec_parameters_copy(st->codecpar, info->m_pCodecPars);
    if (ret < 0) {
        Error("Fail to copy stream infos. Msg: %s\n", ErrToStr(ret).c_str());
        return ret;
    }
    st->codecpar->codec_tag = 0;
    st->time_base = info->m_rTimeBase;

    m_mIdxs[idx] = st->index;
    m_mInfo[idx] = *info;

    return 0;
}

int CFFMux::write(AVPacket* pkt) {
    unsigned idx = pkt->stream_index;
    AVStream *st = m_pFmt->streams[idx];

    if (m_mIdxs.find(idx) == m_mIdxs.end()) {
        Error("No stream %d found.\n", idx);
        return AVERROR(EINVAL);
    }

    if (!m_bInited) {
        int ret = avformat_write_header(m_pFmt, nullptr);
        if (ret < 0) {
            Error("Fail to init muxers. Msg: %s\n", ErrToStr(ret).c_str());
            return ret;
        }
        av_dump_format(m_pFmt, 0, m_sUrl.c_str(), 1);
        m_bInited = true;
    }
    pkt->stream_index = m_mIdxs.at(idx);

    Debug("Mux <- Encoder[%d]: dts = %s, time %s\n", idx, TsToStr(pkt->dts).c_str(),
         TsToTimeStr(pkt->dts, m_mInfo[idx].m_rTimeBase.num, m_mInfo[idx].m_rTimeBase.den).c_str());

    av_packet_rescale_ts(pkt, m_mInfo[idx].m_rTimeBase, st->time_base);

    return av_interleaved_write_frame(m_pFmt, pkt);
}
