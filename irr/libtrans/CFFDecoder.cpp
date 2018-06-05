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

#include "CStreamInfo.h"
#include "CFFDecoder.h"
#include "CQSVDevice.h"
extern "C" {
#include <libavutil/opt.h>
}

CFFDecoder::CFFDecoder(CStreamInfo *info, AVDictionary *pDict) : CTransLog(__func__) {
    int ret = 0;

    m_nFrames = 0;
    m_Info    = *info;
    m_bInited = false;
    m_pCodec  = avcodec_alloc_context3(FindDecoder(info->m_pCodecPars->codec_id));
    if (!m_pCodec) {
        Error("Out of memory.\n");
        return;
    }

    ret = avcodec_parameters_to_context(m_pCodec, m_Info.m_pCodecPars);
    if (ret != 0) {
        Error("Fail to convert codec_par to AVCodecContext, err: %s\n",
              ErrToStr(ret).c_str());
        return;
    }

    m_pCodec->time_base         = m_Info.m_rTimeBase;
    m_pCodec->opaque            = this;
    m_pCodec->get_format        = get_format;
    m_pCodec->framerate         = m_Info.m_rFrameRate;
    if (!av_q2d(m_pCodec->framerate))
        m_pCodec->framerate     = (AVRational){25,1};

    av_opt_set_int(m_pCodec, "refcounted_frames", 1, 0);
    ret = avcodec_open2(m_pCodec, nullptr, &pDict);
    if (ret < 0) {
        Error("Fail to open corresponding decoder. Msg: %s\n", ErrToStr(ret).c_str());
        return;
    }
    avcodec_parameters_from_context(m_Info.m_pCodecPars, m_pCodec);
    m_Info.m_rFrameRate = m_pCodec->framerate;
    m_Info.m_rTimeBase  = m_pCodec->time_base;
}

CFFDecoder::~CFFDecoder() {
    if (m_pCodec)
        avcodec_free_context(&m_pCodec);
    Info("Total decoded frames %d.\n", m_nFrames);
}

AVCodec *CFFDecoder::FindDecoder(AVCodecID id) {
    switch (id) {
        case AV_CODEC_ID_H264:       return avcodec_find_decoder_by_name("h264_qsv");
        case AV_CODEC_ID_MPEG2VIDEO: return avcodec_find_decoder_by_name("mpeg2_qsv");
        case AV_CODEC_ID_H265:       return avcodec_find_decoder_by_name("hevc_qsv");
        default:                     return avcodec_find_decoder(id);
    }
}

AVPixelFormat CFFDecoder::get_format(AVCodecContext *s, const AVPixelFormat *pix_fmts) {
    int                        ret = 0;
    CFFDecoder              *pThis = static_cast<CFFDecoder *>(s->opaque);
    AVPixelFormat          pix_fmt = *pix_fmts;
    AVBufferRef           *pDevice = nullptr;
    AVHWFramesContext   *hw_frames = nullptr;
    AVQSVFramesContext *frames_ctx = nullptr;

    for (; *pix_fmts != AV_PIX_FMT_NONE; pix_fmts++) {
        if (*pix_fmts == AV_PIX_FMT_QSV) {
            pDevice = CQSVDevice::getInstance()->getQsvDev();
            break;
        } else if (*pix_fmts == AV_PIX_FMT_VAAPI) {
            pDevice = CVAAPIDevice::getInstance()->getVaapiDev();
            break;
        }
    }
    if (*pix_fmts == AV_PIX_FMT_NONE)
        return pix_fmt;

    if (!pDevice) {
        pThis->Error("Fail to allocate an HWACCEL device.\n");
        return AV_PIX_FMT_NONE;
    }

    s->hw_frames_ctx = av_hwframe_ctx_alloc(pDevice);
    if (!s->hw_frames_ctx) {
        pThis->Error("Fail to allocate a hwframes ctx.\n");
        return pix_fmt;
    }

    hw_frames  = (AVHWFramesContext *)s->hw_frames_ctx->data;

    hw_frames->width             = FFALIGN(s->coded_width,  32);
    hw_frames->height            = FFALIGN(s->coded_height, 32);
    hw_frames->format            = *pix_fmts;
    hw_frames->sw_format         = s->sw_pix_fmt;
    hw_frames->initial_pool_size = 64;
    if (hw_frames->format == AV_PIX_FMT_QSV) {
        frames_ctx = (AVQSVFramesContext *)hw_frames->hwctx;
        frames_ctx->frame_type   = MFX_MEMTYPE_VIDEO_MEMORY_DECODER_TARGET;
    }

    ret = av_hwframe_ctx_init(s->hw_frames_ctx);
    if (ret < 0) {
        pThis->Error("Fail to init a QSV frames buffer. Msg: %s\n",
                     pThis->ErrToStr(ret).c_str());
        return pix_fmt;
    }

    return AV_PIX_FMT_QSV;
}

int CFFDecoder::write(AVPacket* pkt) {
    int ret = avcodec_send_packet(m_pCodec, pkt);

    if (!m_bInited) {
        /* Update decoder's information after first packet is pushed,
         * as certain decoders has their own parameters those differ from
         * FFmpeg's. Such as qsvdec->format.
         */
        avcodec_parameters_from_context(m_Info.m_pCodecPars, m_pCodec);
        m_Info.m_rFrameRate = m_pCodec->framerate;
        m_Info.m_rTimeBase  = m_pCodec->time_base;
        m_bInited = true;
    }

    return ret;
}

static void null_free(void *opaque, uint8_t *data) {
    ///< Do nothing.
}

int CFFDecoder::write(uint8_t* data, int len) {
    AVPacket avpkt;
    int ret = 0;

    if (!data)
        return write(nullptr);

    av_init_packet(&avpkt);
    avpkt.buf  = av_buffer_create(data, len, null_free, nullptr, AV_BUFFER_FLAG_READONLY);
    avpkt.size = avpkt.buf->size;
    avpkt.data = avpkt.buf->data;

    ret = write(&avpkt);
    av_packet_unref(&avpkt);

    return ret;
}

AVFrame *CFFDecoder::read() {
    int ret = 0;
    AVFrame *frame = av_frame_alloc();

    ret = avcodec_receive_frame(m_pCodec, frame);
    if (ret < 0) {
        if (ret != AVERROR(EAGAIN))
            Error("Unable to get a frame. Error:%s.\n", ErrToStr(ret).c_str());
        av_frame_free(&frame);
        return nullptr;
    }

    frame->pts = av_frame_get_best_effort_timestamp(frame);
    Debug("Decoder -> Filter: dts %s, time %s\n", TsToStr(frame->pts).c_str(),
         TsToTimeStr(frame->pts, m_pCodec->time_base.num, m_pCodec->time_base.den).c_str());

    m_nFrames ++;

    return frame;
}

int CFFDecoder::getNumFrames() {
    return m_nFrames;
}

CStreamInfo* CFFDecoder::getDecInfo() {
    return &m_Info;
}
