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

#include "CFFFilter.h"
#include "CQSVDevice.h"
#include "CVAAPIDevice.h"
#include <string>

using namespace std;

CFFFilter::CFFFilter(CStreamInfo *in, CStreamInfo *out) : CTransLog(__func__) {
    char buffer[1024] = "\0";
    AVFilterContext *pFrc;
    AVFilterContext *pHwupload;
    AVBufferRef *pHwDev;

    m_pGraph   = avfilter_graph_alloc();
    m_nFrames  = 0;
    m_bInited  = false;
    m_SrcInfo  = *in;
    m_SinkInfo = *out;
    m_pSrc     = m_pSink = nullptr;

    if (in->m_pCodecPars->codec_type == AVMEDIA_TYPE_VIDEO) {
        ///< Create buffersrc
        snprintf(buffer, sizeof(buffer),
                "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:sar=%d/%d",
                in->m_pCodecPars->width, in->m_pCodecPars->height,
                in->m_pCodecPars->format,
                1, AV_TIME_BASE,
                in->m_pCodecPars->sample_aspect_ratio.num, in->m_pCodecPars->sample_aspect_ratio.den);
        if (av_q2d(in->m_rFrameRate)) {
            snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer),
                     ":frame_rate=%d/%d",
                     in->m_rFrameRate.num, in->m_rFrameRate.den);
        }
        m_pSrc  = alloc_filter("buffer", buffer);
        if (!m_pSrc) {
            Error("Fail to create filter %s.\n", "buffer");
            return;
        }

        ///< Create buffersink
        snprintf(buffer, sizeof(buffer), "pix_fmts=%02x%02x%02x%02x",
                 out->m_pCodecPars->format & 0xFF, (out->m_pCodecPars->format >> 8)&0xFF, 0, 0);
        m_pSink = alloc_filter("buffersink", buffer);
        if (!m_pSink) {
            Error("Fail to create filter %s.\n", "buffersink");
            return;
        }

        ///< Create FRC
        snprintf(buffer, sizeof(buffer), "%d/%d",
                 m_SinkInfo.m_rFrameRate.num, m_SinkInfo.m_rFrameRate.den);
        pFrc = alloc_filter("fps", buffer);
        if (!pFrc) {
            Error("Fail to create filter %s.\n", "fps");
            return;
        }

        ///< Create Filter pipeline
        avfilter_link(m_pSrc, 0, pFrc, 0);
        avfilter_link(pFrc, 0, m_pSink, 0);

        ///< Check if Converting to Video-Memory is needed
        if (out->m_pCodecPars->format == AV_PIX_FMT_QSV ||
            out->m_pCodecPars->format == AV_PIX_FMT_VAAPI) {
            pHwupload = alloc_filter("hwupload", nullptr);
            if (!pHwupload) {
                Error("Fail to create filter %s.\n", "hwupload");
                return;
            }

            avfilter_insert_filter(m_pSink->inputs[0], pHwupload, 0, 0);

            if (out->m_pCodecPars->format == AV_PIX_FMT_QSV)
                pHwDev = CQSVDevice::getInstance()->getQsvDev();
            else
                pHwDev = CVAAPIDevice::getInstance()->getVaapiDev();

            for (unsigned idx = 0; idx < m_pGraph->nb_filters; idx++)
                m_pGraph->filters[idx]->hw_device_ctx = av_buffer_ref(pHwDev);
        }
    } else if (in->m_pCodecPars->codec_type == AVMEDIA_TYPE_AUDIO) {
        snprintf(buffer, sizeof(buffer),
                 "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channels=%d:channel_layout=%d",
                 1, in->m_pCodecPars->sample_rate, in->m_pCodecPars->sample_rate,
                 av_get_sample_fmt_name((AVSampleFormat)in->m_pCodecPars->format),
                 in->m_pCodecPars->channels,
                 in->m_pCodecPars->channels > 1 ? AV_CH_LAYOUT_STEREO : AV_CH_LAYOUT_MONO);
        m_pSrc  = alloc_filter("abuffer", buffer);
        if (!m_pSrc) {
            Error("Fail to create filter %s.\n", "abuffer");
            return;
        }

        snprintf(buffer, sizeof(buffer), "sample_fmts=%02x%02x%02x%02x:sample_rates=%02x%02x%02x%02x",
                 out->m_pCodecPars->format & 0xFF, (out->m_pCodecPars->format >> 8)&0xFF, 0, 0,
                 out->m_pCodecPars->sample_rate & 0xFF, (out->m_pCodecPars->sample_rate >> 8)&0xFF,
                 (out->m_pCodecPars->sample_rate >> 16)&0xFF, (out->m_pCodecPars->sample_rate >> 24)&0xFF);
        m_pSink = alloc_filter("abuffersink", buffer);
        if (!m_pSink) {
            Error("Fail to create filter %s.\n", "abuffersink");
            return;
        }

        avfilter_link(m_pSrc, 0, m_pSink, 0);
    }
}

CFFFilter::~CFFFilter() {
    avfilter_graph_free(&m_pGraph);
    Info("Total filtered frames: %d\n", m_nFrames);
}

AVFilterContext *CFFFilter::alloc_filter(const char* name, const char* par) {
    AVFilterContext *pFilter;
    int ret = avfilter_graph_create_filter(&pFilter, avfilter_get_by_name(name), name,
                                           par, this, m_pGraph);
    if (ret < 0) {
        Error("Fail to create %s. Msg: %s\n", name, ErrToStr(ret).c_str());
        return nullptr;
    }
    return pFilter;
}

int CFFFilter::push(AVFrame* frame) {
    int ret = 0;

    if (!m_bInited) {
        AVBufferSrcParameters *par = av_buffersrc_parameters_alloc();

        par->format              = frame->format;
        par->width               = frame->width;
        par->height              = frame->height;
        par->sample_aspect_ratio = frame->sample_aspect_ratio;
        par->sample_rate         = frame->sample_rate;
        par->channel_layout      = frame->channel_layout;
        if (frame->hw_frames_ctx)
            par->hw_frames_ctx   = frame->hw_frames_ctx;
        ret = av_buffersrc_parameters_set(m_pSrc, par);
        av_freep(&par);
        if (ret < 0) {
            Error("Fail to reset parameters. Error '%s'\n", ErrToStr(ret).c_str());
            return ret;
        }

        ret = avfilter_graph_config(m_pGraph, nullptr);
        if (ret < 0) {
            Error("Fail to config filters. Error '%s'\n", ErrToStr(ret).c_str());
            return ret;
        }

        /* Update stream info after configuring. */
        m_SrcInfo.m_rTimeBase                        = m_pSrc->outputs[0]->time_base;
        m_SinkInfo.m_rFrameRate                      = av_buffersink_get_frame_rate(m_pSink);
        m_SinkInfo.m_rTimeBase                       = av_buffersink_get_time_base(m_pSink);
        m_SinkInfo.m_pCodecPars->format              = av_buffersink_get_format(m_pSink);
        m_SinkInfo.m_pCodecPars->width               = av_buffersink_get_w(m_pSink);
        m_SinkInfo.m_pCodecPars->height              = av_buffersink_get_h(m_pSink);
        m_SinkInfo.m_pCodecPars->sample_aspect_ratio = av_buffersink_get_sample_aspect_ratio(m_pSink);
        m_SinkInfo.m_pCodecPars->channels            = av_buffersink_get_channels(m_pSink);
        m_SinkInfo.m_pCodecPars->sample_rate         = av_buffersink_get_sample_rate(m_pSink);
        m_SinkInfo.m_pCodecPars->channel_layout      = av_buffersink_get_channel_layout(m_pSink);
        m_bInited = true;
    }

    Debug("Filter <- Decoder: dts %s, time %s\n", TsToStr(frame->pts).c_str(),
         TsToTimeStr(frame->pts, m_pSrc->outputs[0]->time_base.num,
                     m_pSrc->outputs[0]->time_base.den).c_str());

    return av_buffersrc_add_frame(m_pSrc, frame);
}

AVFrame *CFFFilter::pop(void) {
    int ret = 0;
    AVFrame *frame = av_frame_alloc();
    if (!m_bInited) {
        av_frame_free(&frame);
        return nullptr;
    }

    ret = av_buffersink_get_frame(m_pSink, frame);
    if (ret < 0) {
        av_frame_free(&frame);
        return nullptr;
    }

    m_nFrames ++;

    Debug("Filter -> Encoder: dts %s, time %s\n", TsToStr(frame->pts).c_str(),
          TsToTimeStr(frame->pts, m_pSink->inputs[0]->time_base.num,
                     m_pSink->inputs[0]->time_base.den).c_str());

    return frame;
}

int CFFFilter::getNumFrames() {
    return m_nFrames;
}

CStreamInfo* CFFFilter::getSinkInfo() {
    return &m_SinkInfo;
}

CStreamInfo* CFFFilter::getSrcInfo() {
    return &m_SrcInfo;
}
