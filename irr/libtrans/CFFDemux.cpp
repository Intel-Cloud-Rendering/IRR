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

#include "CFFDemux.h"

using namespace std;

CFFDemux::CFFDemux(const char *url) : CTransLog(__func__) {
    m_pDemux = nullptr;
    m_sUrl   = url;
}

CFFDemux::~CFFDemux() {
    for (auto it:m_mStreamInfos)
        delete it.second;

    if (m_pDemux) {
        avformat_close_input(&m_pDemux);
    }
}

int CFFDemux::init(const char *url, const char *format, AVDictionary *pDict) {
    int                ret = 0;
    AVInputFormat *iformat = nullptr;

    if (format) {
        iformat = av_find_input_format(format);
        if (!iformat)
            Warn("Requested format %s is not found.\n", format);
    }

    ret = avformat_open_input(&m_pDemux, url, iformat, &pDict);
    if (ret < 0) {
        Error("Open input %s failure. Error: %s.\n", url, ErrToStr(ret).c_str());
        return ret;
    }

    ret = avformat_find_stream_info(m_pDemux, NULL);
    if (ret < 0) {
        Error("Find stream info for input %s failure. Error: %s.\n", url, ErrToStr(ret).c_str());
        return ret;
    }
    av_dump_format(m_pDemux, 0, url, 0);

    for (unsigned idx = 0; idx < m_pDemux->nb_streams; idx ++) {
        CStreamInfo *info = new CStreamInfo();
        avcodec_parameters_copy(info->m_pCodecPars, m_pDemux->streams[idx]->codecpar);
        info->m_rFrameRate = m_pDemux->streams[idx]->avg_frame_rate;
        info->m_rTimeBase  = m_pDemux->streams[idx]->time_base;
        m_mStreamInfos[idx] = info;
    }

    return 0;
}

int CFFDemux::start(const char *format, AVDictionary *pDict) {
    return init(m_sUrl.c_str(), format, pDict);
}

int CFFDemux::getNumStreams() {
    return m_pDemux ? m_pDemux->nb_streams : 0;
}

CStreamInfo* CFFDemux::getStreamInfo(int strIdx) {
    return m_mStreamInfos[strIdx];
}

int CFFDemux::readPacket(AVPacket* avpkt) {
    int ret;

    if (!m_pDemux)
        return AVERROR(EINVAL);

    ret = av_read_frame(m_pDemux, avpkt);
    if (ret < 0)
        return ret;

    if (m_mStreamInfos.find(avpkt->stream_index) == m_mStreamInfos.end()) {
        CStreamInfo *info = new CStreamInfo();
        int           idx = avpkt->stream_index;

        Warn("New stream is found.\n");

        avcodec_parameters_copy(info->m_pCodecPars, m_pDemux->streams[idx]->codecpar);
        info->m_rFrameRate = m_pDemux->streams[idx]->avg_frame_rate;
        info->m_rTimeBase  = m_pDemux->streams[idx]->time_base;
        m_mStreamInfos[avpkt->stream_index] = info;
    }

    Debug("Demux -> Decoder[%d]: dts %s, time %s\n", avpkt->stream_index,
         TsToStr(avpkt->dts).c_str(),
         TsToTimeStr(avpkt->dts, m_mStreamInfos[avpkt->stream_index]->m_rTimeBase.num,
                     m_mStreamInfos[avpkt->stream_index]->m_rTimeBase.den).c_str());

    return ret;
}

int CFFDemux::seek(long offset, int whence) {
    return m_pDemux ? avio_seek(m_pDemux->pb, offset, whence) : AVERROR(EINVAL);
}

int CFFDemux::tell() {
    return m_pDemux ? avio_tell(m_pDemux->pb) : AVERROR(EINVAL);
}

int CFFDemux::size() {
    return m_pDemux ? avio_size(m_pDemux->pb) : AVERROR(EINVAL);
}
