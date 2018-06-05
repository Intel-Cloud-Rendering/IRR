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

#include "CFFEncoder.h"

using namespace std;

CFFEncoder::CFFEncoder(const char *pCodec, CStreamInfo *info, AVDictionary *pDict) : CTransLog(__func__) {
    int ret;
    AVCodec *codec = avcodec_find_encoder_by_name(pCodec);

    if (!codec)
        codec = FindEncoder(info->m_pCodecPars->codec_id);

    if (!codec)
        Warn("Can't find any encoder by codec %s or codec id %d.\n", pCodec, info->m_pCodecPars->codec_id);

    m_bInited = false;
    m_pDict   = nullptr;
    m_nFrames = 0;
    m_Info    = *info;
    m_pEnc    = avcodec_alloc_context3(codec);

    ret = avcodec_parameters_to_context(m_pEnc, info->m_pCodecPars);
    if (ret < 0) {
        Error("Invalid streaminfo. Msg: %s\n", ErrToStr(ret).c_str());
        return;
    }

    m_pEnc->framerate     = info->m_rFrameRate;
    m_pEnc->time_base     = info->m_rTimeBase;
    m_pEnc->gop_size      = 255;
    m_pEnc->max_b_frames  = 0;
    m_pEnc->refs          = 2;

    av_dict_set(&m_pDict, "preset", "veryfast", AV_DICT_DONT_OVERWRITE);
    av_dict_copy(&m_pDict, pDict, 0);
}

CFFEncoder::~CFFEncoder() {
    av_dict_free(&m_pDict);
    avcodec_free_context(&m_pEnc);
    Info("Total Encoded frames: %d\n", m_nFrames);
}

AVCodec *CFFEncoder::FindEncoder(AVCodecID id) {
    switch (id) {
        case AV_CODEC_ID_H264:       return avcodec_find_encoder_by_name("h264_vaapi");
        case AV_CODEC_ID_MPEG2VIDEO: return avcodec_find_encoder_by_name("mpeg2_vaapi");
        case AV_CODEC_ID_H265:       return avcodec_find_encoder_by_name("hevc_vaapi");
        default:                     return avcodec_find_encoder(id);
    }
}

int CFFEncoder::write(AVFrame *pFrame) {
    int ret = 0;

    if (!m_bInited) {
        if (!pFrame) {
            Error("Give a null as first frame ???\n");
            return AVERROR(EINVAL);
        }

        if (!m_pEnc->codec) {
            Error("No encoder is specified, EOF.\n");
            return AVERROR_EOF;
        }

        if (pFrame->hw_frames_ctx)
            m_pEnc->hw_frames_ctx = av_buffer_ref(pFrame->hw_frames_ctx);

        ret = avcodec_open2(m_pEnc, nullptr, &m_pDict);
        if (ret < 0) {
            Error("Fail to open encoder. Msg: %s\n", ErrToStr(ret).c_str());
            return ret;
        }

        avcodec_parameters_from_context(m_Info.m_pCodecPars, m_pEnc);
        m_Info.m_rFrameRate = m_pEnc->framerate;
        m_Info.m_rTimeBase  = m_pEnc->time_base;

        m_bInited = true;
    }

    if (pFrame)
        Debug("Encoder <- Filter: dts %s, time %s\n", TsToStr(pFrame->pts).c_str(),
              TsToTimeStr(pFrame->pts, m_pEnc->time_base.num, m_pEnc->time_base.den).c_str());
    ret = avcodec_send_frame(m_pEnc, pFrame);
    if (ret < 0) {
        if (ret != AVERROR_EOF)
            Error("Fail to push a frame into encoder. Msg: %s.\n", ErrToStr(ret).c_str());
        return ret;
    }

    return ret;
}

int CFFEncoder::read(AVPacket *pPkt) {
    int ret = avcodec_receive_packet(m_pEnc, pPkt);

    if (ret >= 0)
        m_nFrames++;

    return ret;
}

CStreamInfo *CFFEncoder::getStreamInfo() {
    return &m_Info;
}

int CFFEncoder::GetBestFormat(const char *codec, int format)
{
    AVCodec *pCodec = avcodec_find_encoder_by_name(codec);
    if (!pCodec)
        return -1;

    if (pCodec->type == AVMEDIA_TYPE_VIDEO) {
        const AVPixelFormat *pFormat = pCodec->pix_fmts;
        for (; *pFormat != AV_PIX_FMT_NONE; pFormat++)
            if (*pFormat == format)
                return format;

        return pCodec->pix_fmts[0];
    } else if (pCodec->type == AVMEDIA_TYPE_AUDIO) {
        const AVSampleFormat *pFormat = pCodec->sample_fmts;
        for (; *pFormat != AV_SAMPLE_FMT_NONE; pFormat++)
            if (*pFormat == format)
                return format;

        return pCodec->sample_fmts[0];
    }

    return format;
}
