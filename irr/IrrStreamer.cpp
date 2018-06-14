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

#include "stream.h"
#include "IrrStreamer.h"
#include "libtrans/CCallbackMux.h"

using namespace std;

IrrStreamer::IrrStreamer(int w, int h, float framerate) : CTransLog(__func__){
    m_nPixfmt    = AV_PIX_FMT_RGBA;
    m_nMaxPkts   = 5;
    m_nCurPkts   = 0;
    m_pTrans     = nullptr;
    m_pDemux     = nullptr;
    m_pPool      = nullptr;
    m_nWidth     = w;
    m_nHeight    = h;
    m_fFramerate = framerate;
    m_nFrameSize = av_image_get_buffer_size(m_nPixfmt, w, h, 32);
    m_pPool      = av_buffer_pool_init2(m_nFrameSize, this, m_BufAlloc, nullptr);
}

IrrStreamer::~IrrStreamer() {
    stop();
    av_buffer_pool_uninit(&m_pPool);
}

int IrrStreamer::start(IrrStreamInfo *param) {
    CMux *pMux = nullptr;
    lock_guard<mutex> lock(m_Lock);

    if (!param) {
        Error("No stream information.\n");
        return AVERROR(EINVAL);
    }

    if (m_pTrans) {
        Error("Already running.\n");
        return AVERROR(EINVAL);
    }

    m_pDemux = new CIrrVideoDemux(m_nWidth, m_nHeight, m_nPixfmt, m_fFramerate);
    if (!m_pDemux) {
        Error("Fail to create Demux : Out of memory.\n");
        return AVERROR(ENOMEM);
    }

    if (param->url && strcmp(param->url, "callback")) {
        m_pTrans = new CTransCoder(m_pDemux, param->url);
    } else {
        pMux = new CCallbackMux(param->cb_params.opaque, param->cb_params.cbOpen,
                                param->cb_params.cbWrite, param->cb_params.cbClose);
        if (!pMux) {
            Error("Out of memory.\n");
            delete m_pDemux;
            return AVERROR(ENOMEM);
        }

        m_pTrans = new CTransCoder(m_pDemux,pMux);
    }

    if (!m_pTrans) {
        Error("Out of memory.\n");
        delete m_pDemux;
        if (pMux) delete pMux;

        return AVERROR(ENOMEM);
    }

#define SETOPTINT(name, val) { \
char buf[32]; \
snprintf(buf, sizeof(buf), "%d", val); \
m_pTrans->setOutputProp(name, buf); \
}

    //if (!param->bitrate)
    //    param->bitrate = 2000000;
    //SETOPTINT("b", param->bitrate);///< Bitrate

    if (param->codec)
        m_pTrans->setOutputProp("c", param->codec); ///< Codec
    else
        m_pTrans->setOutputProp("c", "h264_vaapi");

    if (param->framerate)
        m_pTrans->setOutputProp("r", param->framerate); ///< Framerate

    if (param->low_power)
        m_pTrans->setOutputProp("low_power", "1");

    if (param->res)
        m_pTrans->setOutputProp("s", param->res);

    if (param->gop_size)
        SETOPTINT("g", param->gop_size); ///< GOP size

    if (param->exp_vid_param) {
        string expar = param->exp_vid_param;
        do {
            string kv;
            string::size_type nSep = expar.find_first_of(':');
            if (nSep == string::npos) {
                kv = expar;
                expar.resize(0);
            } else {
                kv = expar.substr(0, nSep);
                expar.erase(0, nSep + 1);
            }

            string::size_type nKvsep = kv.find_first_of('=');
            if (nKvsep == string::npos)
                Debug("%s is not a key-value pair.\n", kv.c_str());
            else {
                Debug("Find '=' at kv[%lu], key = '%s', value='%s'\n",
                       nKvsep, kv.substr(0, nKvsep).c_str(), kv.substr(nKvsep + 1).c_str());
                m_pTrans->setOutputProp(kv.substr(0, nKvsep).c_str(), kv.substr(nKvsep + 1).c_str());
            }
        } while (expar.size());
    }

    return m_pTrans->start();
}

void IrrStreamer::stop() {
    lock_guard<mutex> lock(m_Lock);
    delete m_pTrans;
    m_pTrans = nullptr;
    ///< Demux will be released by CTransCoder's deconstuctor.
    m_pDemux = nullptr;
    m_mPkts.clear();
}

int IrrStreamer::write(const void *data, int size) {
    lock_guard<mutex> lock(m_Lock);
    auto it = m_mPkts.find(data);

    if (it != m_mPkts.end() && m_pDemux) {
        AVPacket pkt;

        av_init_packet(&pkt);
        pkt.buf  = it->second;
        pkt.data = pkt.buf->data;
        pkt.size = pkt.buf->size;
        pkt.stream_index = 0;
        m_mPkts.erase(it);
        return m_pDemux->sendPacket(&pkt);
    } else if (it != m_mPkts.end()) {
        av_buffer_unref(&it->second);
        m_mPkts.erase(it);
        return 0;
    } else if (!m_pDemux)
        return 0;

    /*
     * Corresponding buffer is not found.
     * Request a new buffer for it.
     */
    void *buf = getBuffer();
    if (!buf)
        return AVERROR(ENOMEM);

    memcpy(buf, data, size);
    return write(buf, size);
}

void *IrrStreamer::getBuffer() {
    AVBufferRef *pBuf;

    if (!m_pPool)
        return nullptr;

    pBuf = av_buffer_pool_get(m_pPool);
    if (!pBuf) {
        Error("No free buffer now.\n");
        return nullptr;
    }

    m_mPkts[pBuf->data] = pBuf;
    return pBuf->data;
}

AVBufferRef* IrrStreamer::m_BufAlloc(void *opaque, int size) {
    IrrStreamer *pThis = static_cast<IrrStreamer*> (opaque);

    /* Check if the number of packets reaches the limit */
    if (pThis->m_nCurPkts >= pThis->m_nMaxPkts)
        return nullptr;

    return av_buffer_alloc(size);
}

int IrrStreamer::force_key_frame(int force_key_frame) {
    lock_guard<mutex> lock(m_Lock);

    if(m_pTrans==nullptr) {
        Error("transcoder is not started\n");
        return -1;
    }

    if(force_key_frame) {
        m_pTrans->forceKeyFrame(1);
    }

    return 0;
}
