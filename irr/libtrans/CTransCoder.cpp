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

#include "CTransCoder.h"
#include "CThread.hpp"
#include "CFFDemux.h"
#include "CFFDecoder.h"
#include "CFFFilter.h"
#include "CFFEncoder.h"
#include "CTransLog.h"
#include "CFFMux.h"

extern "C" {
#include <libavutil/time.h>
#include <libavutil/pixdesc.h>
#include <libavutil/parseutils.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/avstring.h>
}

using namespace std;

CTransCoder::CTransCoder(string sSrcUrl, string sDstUrl, string sDstFormat) {
    pthread_mutex_init(&m_IntMutex, nullptr);
    m_bInterrupt = false;
    m_pThread    = new CThread(this);
    m_Log        = new CTransLog(__func__);
    m_pDemux     = new CFFDemux(sSrcUrl.c_str());
    m_pMux       = new CFFMux(sDstUrl.c_str(), sDstFormat.c_str());
    m_pInProp    = nullptr;
    m_pOutProp   = nullptr;
}

CTransCoder::CTransCoder(std::string sSrcUrl, CMux *pMux) {
    pthread_mutex_init(&m_IntMutex, nullptr);
    m_bInterrupt = false;
    m_pThread    = new CThread(this);
    m_Log        = new CTransLog(__func__);
    m_pDemux     = new CFFDemux(sSrcUrl.c_str());
    m_pMux       = pMux;
    m_pInProp    = nullptr;
    m_pOutProp   = nullptr;
}

CTransCoder::CTransCoder(CDemux *pDemux, CMux *pMux) {
    pthread_mutex_init(&m_IntMutex, nullptr);
    m_bInterrupt = false;
    m_pThread    = new CThread(this);
    m_Log        = new CTransLog(__func__);
    m_pDemux     = pDemux;
    m_pMux       = pMux;
    m_pInProp    = nullptr;
    m_pOutProp   = nullptr;
}

CTransCoder::CTransCoder(CDemux *pDemux, string sDstUrl, string sDstFormat) {
    pthread_mutex_init(&m_IntMutex, nullptr);
    m_bInterrupt = false;
    m_pThread    = new CThread(this);
    m_Log        = new CTransLog(__func__);
    m_pDemux     = pDemux;
    m_pMux       = new CFFMux(sDstUrl.c_str(), sDstFormat.c_str());
    m_pInProp    = nullptr;
    m_pOutProp   = nullptr;
}

CTransCoder::~CTransCoder() {
    if (m_pThread->status())
        stop();

    av_dict_free(&m_pInProp);
    av_dict_free(&m_pOutProp);

    if (pthread_mutex_destroy(&m_IntMutex) < 0)
        m_Log->Error("Fail to call pthread_mutex_destroy!\n");

    delete m_pMux;
    for (auto it:m_mEncoders)
        delete it.second;
    for (auto it:m_mFilters)
        delete it.second;
    for (auto it:m_mDecoders)
        delete it.second;
    delete m_Log;
    delete m_pDemux;
    delete m_pThread;
}

int CTransCoder::start() {
    int ret;
    const char *pVal = getInOptVal("f", "format");

    if (m_pThread->status())
        return 0;

    ret = m_pDemux->start(pVal, m_pInProp);
    if (ret < 0)
        return ret;

    return m_pThread->start();
}

void CTransCoder::wait() {
    m_pThread->wait();
}

void CTransCoder::stop() {
    doOutput(true);
    m_pThread->stop();
}

int CTransCoder::setInputProp(const char *key, const char *value) {
    return av_dict_set(&m_pInProp, key, value, 0);
}

int CTransCoder::setOutputProp(const char *key, const char *value) {
    return av_dict_set(&m_pOutProp, key, value, 0);
}

bool CTransCoder::interrupt_callback() {
    CAutoLock lock(&m_IntMutex);
    return m_bInterrupt;
}

void CTransCoder::interrupt() {
    CAutoLock lock(&m_IntMutex);
    m_bInterrupt = true;
}

void CTransCoder::run() {
    int ret = processInput();
    if (ret < 0) {
        if (ret == AVERROR_EOF)
            doOutput(true);
        interrupt();
        return;
    }

    ret = doOutput(false);
    if (ret < 0) {
        return;
    }
}

bool CTransCoder::allStreamFound() {
    for (auto it:m_mStreamFound)
        if (!it.second)
            return false;
    return true;
}

int CTransCoder::newInputStream(int strIdx) {
    int ret = 0;
    CStreamInfo *pDemuxInfo;

    if (m_mStreamFound.find(strIdx) == m_mStreamFound.end())
        m_mStreamFound[strIdx] = false;

    if (m_mStreamFound.at(strIdx))
        return 0;

    pDemuxInfo = m_pDemux->getStreamInfo(strIdx);
    switch (pDemuxInfo->m_pCodecPars->codec_type) {
        case AVMEDIA_TYPE_VIDEO:
        case AVMEDIA_TYPE_AUDIO:
            /* New stream found. Init a decoder for it. */
            m_mDecoders[strIdx] = new CFFDecoder(pDemuxInfo, m_pInProp);
            if (!m_mDecoders[strIdx]) {
                m_Log->Error("OOM!!!\n");
                return -1;
            }
            break;

        default:
            /* Force it to found */
            m_mStreamFound[strIdx] = true;
            m_Log->Warn("%s stream found. Discard it.\n",
                        av_get_media_type_string(pDemuxInfo->m_pCodecPars->codec_type));
            break;
    }

    return ret;
}

int CTransCoder::decode(int strIdx) {
    CDecoder       *pDec  = m_mDecoders[strIdx];
    CFilter        *pFilt = nullptr;
    CStreamInfo *pSrcInfo = nullptr;
    CStreamInfo *pDecInfo = pDec->getDecInfo();
    AVFrame *decFrame;
    int ret;

    /* Init filter after first frame is decoded. */
    if (m_mFilters.find(strIdx) == m_mFilters.end()) {
        CStreamInfo EncInfo = *pDecInfo;
        const char *pVal;

        EncInfo.m_pCodecPars->codec_id = AV_CODEC_ID_NONE;
        EncInfo.m_pCodecPars->codec_tag = -1;
        switch (EncInfo.m_pCodecPars->codec_type) {
            case AVMEDIA_TYPE_VIDEO:
#if 0 ///< Not supported by now
                pVal = getOutOptVal("s", "video_size");
                if (pVal) {
                    ret = av_parse_video_size(&EncInfo.m_pCodecPars->width,
                                              &EncInfo.m_pCodecPars->height,
                                              pVal);
                    if (ret < 0) {
                        m_Log->Error("Bad parameters -s/-video_size %s\n", pVal);
                        return ret;
                    }
                }
#endif

                pVal = getOutOptVal("r", "framerate");
                if (pVal) {
                    ret = av_parse_video_rate(&EncInfo.m_rFrameRate,
                                              pVal);
                    if (ret < 0) {
                        m_Log->Error("Bad parameters -r/-framerate %s\n", pVal);
                        return ret;
                    }
                }

                pVal = getOutOptVal("c", "codec", "h264_vaapi");
                if (pVal) {
                    if (av_stristr(pVal, "_vaapi"))
                        EncInfo.m_pCodecPars->format = AV_PIX_FMT_VAAPI;
                }

                EncInfo.m_pCodecPars->format = CFFEncoder::GetBestFormat(pVal, EncInfo.m_pCodecPars->format);
                break;
            case AVMEDIA_TYPE_AUDIO:
                pVal = getOutOptVal("sr", "sample_rate");
                if (pVal)
                    EncInfo.m_pCodecPars->sample_rate = strtol(pVal, nullptr, 10);

                pVal = getOutOptVal("channles", "channles");
                if (pVal)
                    EncInfo.m_pCodecPars->channels = strtol(pVal, nullptr, 10);

                pVal = getOutOptVal("ac", "acodec", "aac");
                EncInfo.m_pCodecPars->format = CFFEncoder::GetBestFormat(pVal, EncInfo.m_pCodecPars->format);
                break;
            default: break;
        }
        m_mFilters[strIdx]  = new CFFFilter(pDecInfo, &EncInfo);
    }

    pFilt    = m_mFilters[strIdx];
    pSrcInfo = pFilt->getSrcInfo();
    while ((decFrame = pDec->read()) != nullptr) {
        decFrame->pts = av_rescale_q(decFrame->pts, pDecInfo->m_rTimeBase,
                                     pSrcInfo->m_rTimeBase);
        decFrame->pict_type = AV_PICTURE_TYPE_NONE;

        ret = pFilt->push(decFrame);
        if (ret < 0) {
            m_Log->Error("Fail to push frame into filters. Msg: %s\n",
                         m_Log->ErrToStr(ret).c_str());
            av_frame_free(&decFrame);
            return ret;
        }
        av_frame_free(&decFrame);
    }

    return 0;
}

int CTransCoder::processInput() {
    int                 ret = 0;
    AVPacket           *pkt = av_packet_alloc();
    CDecoder          *pDec = nullptr;
    CStreamInfo   *pDecInfo = nullptr;
    CStreamInfo *pDemuxInfo = nullptr;

    ret = m_pDemux->readPacket(pkt);
    if (ret < 0) {
        if (ret == AVERROR_EOF) {
            m_Log->Warn("Eof detected. Exiting...\n");
            /* Flush decoder */
            for (auto it:m_mDecoders) {
                it.second->write(nullptr);
                decode(it.first);
            }
        } else
            m_Log->Error("Fail to read a pkt. Msg: %s.\n", m_Log->ErrToStr(ret).c_str());
        goto err_out;
    }

    if (m_mStreamFound.find(pkt->stream_index) == m_mStreamFound.end()) {
        /* New stream found, discard packet */
        m_Log->Warn("New stream found.\n");
        ret = newInputStream(pkt->stream_index);
        if (ret < 0)
            goto err_out;
    }

    pDemuxInfo = m_pDemux->getStreamInfo(pkt->stream_index);
    if (m_mDecoders.find(pkt->stream_index) == m_mDecoders.end()) {
        m_Log->Debug("Disgarding packet from stream[%d], type %s.\n", pkt->stream_index,
                     av_get_media_type_string(pDemuxInfo->m_pCodecPars->codec_type));
        goto err_out;
    }

    pDec     = m_mDecoders[pkt->stream_index];
    pDecInfo = pDec->getDecInfo();

    av_packet_rescale_ts(pkt, pDemuxInfo->m_rTimeBase, pDecInfo->m_rTimeBase);
    ret = pDec->write(pkt);
    if (ret < 0) {
        m_Log->Error("Fail to write packet into Decoder[%d]. Msg: %s.\n",
                     pkt->stream_index, m_Log->ErrToStr(ret).c_str());
        goto err_out;
    }

    ret = decode(pkt->stream_index);
    if (ret < 0) {
        m_Log->Error("Decode[%d] packet failure. Msg: %s.\n",
                     pkt->stream_index, m_Log->ErrToStr(ret).c_str());
        goto err_out;
    }

err_out:
    av_packet_free(&pkt);
    return ret;
}

int CTransCoder::doOutput(bool flush) {
    for (auto it:m_mFilters) {
        int        idx = it.first;
        CFilter *pFilt = it.second;
        CEncoder *pEnc = nullptr;
        AVFrame *pFrame;
        AVPacket pkt;
        int ret;

        if (m_mEncoders.find(idx) == m_mEncoders.end()) {
            const char *pVal;
            CStreamInfo *pSinkInfo = pFilt->getSinkInfo();

            if (pSinkInfo->m_pCodecPars->codec_type == AVMEDIA_TYPE_VIDEO)
                pVal = getOutOptVal("c", "codec", "h264_vaapi"/*"h264_qsv"*/);
            else
                pVal = getOutOptVal("ac", "acodec", "aac");
            m_mEncoders[idx] = new CFFEncoder(pVal, pSinkInfo, m_pOutProp);
        }

        av_init_packet(&pkt);
        pEnc = m_mEncoders[idx];
        while ((pFrame = pFilt->pop()) != nullptr || flush) {
            if (!pFrame && flush) {
                pEnc->write(nullptr);
            } else {
                ret = pEnc->write(pFrame);
                if (ret < 0) {
                    if (ret != AVERROR_EOF)
                        m_Log->Error("Fail to encode a frame. Msg: %s.\n",
                                     m_Log->ErrToStr(ret).c_str());
                    else
                        m_Log->Warn("Eof detected. Exiting...\n");

                    av_frame_free(&pFrame);
                    return ret;
                }
                av_frame_free(&pFrame);

                if (!m_mStreamFound.at(idx)) {
                    ret = m_pMux->addStream(idx, pEnc->getStreamInfo());
                    if (ret < 0) {
                        m_Log->Error("Fail to add stream.\n");
                        return ret;
                    }
                    m_mStreamFound[idx] = true;
                }

                if (!allStreamFound()) continue;
            }

            while ((ret = pEnc->read(&pkt)) >= 0) {
                pkt.stream_index = idx;
                ret = m_pMux->write(&pkt);
                if (ret < 0) {
                    av_packet_unref(&pkt);
                    m_Log->Error("Fail to output a frame.\n");
                    return ret;
                }
                av_packet_unref(&pkt);
            }

            if (!pFrame && flush) break;
        }
    }

    return 0;
}

const char* CTransCoder::getInOptVal(const char *short_name, const char *long_name,
                                   const char *default_value) {
    AVDictionaryEntry *dEntry = nullptr;

    if ((dEntry = av_dict_get(m_pInProp, short_name, nullptr, 0)) != nullptr ||
        (dEntry = av_dict_get(m_pInProp, long_name, nullptr, 0)) != nullptr)
        return dEntry->value;

    return default_value;
}

const char* CTransCoder::getOutOptVal(const char *short_name, const char *long_name,
                                   const char *default_value) {
    AVDictionaryEntry *dEntry = nullptr;

    if ((dEntry = av_dict_get(m_pOutProp, short_name, nullptr, 0)) != nullptr ||
        (dEntry = av_dict_get(m_pOutProp, long_name, nullptr, 0)) != nullptr)
        return dEntry->value;

    return default_value;
}
