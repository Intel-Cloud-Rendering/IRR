
#include "stream.h"
#include <map>
#include <memory>
#include "android/base/synchronization/Lock.h"
#include "libtrans/CTransCoder.h"
ANDROID_BEGIN_HEADER
#include <libavutil/time.h>
#include <libavutil/imgutils.h>
ANDROID_END_HEADER

using namespace std;

class IrrVideoDemux : public CDemux {
public:
    IrrVideoDemux(int w, int h, int format, float framerate) {
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

    ~IrrVideoDemux() {
        av_packet_unref(&m_Pkt);
    }

    int getNumStreams() { return 1;}

    CStreamInfo* getStreamInfo(int strIdx) {
        return &m_Info;
    }

    int readPacket(AVPacket *avpkt) {
        int ret;

        while (av_gettime_relative() - m_nStartTime < m_nNextPts)
            av_usleep(1000);

        android::base::AutoLock mutex(mLock);

        m_Pkt.pts  = m_Pkt.dts = m_nNextPts;
        ret = av_packet_ref(avpkt, &m_Pkt);

        m_nNextPts += av_rescale_q(1, av_inv_q(m_Info.m_rFrameRate), m_Info.m_rTimeBase);

        return ret;
    }

    int sendPacket(AVPacket *pkt) {
        android::base::AutoLock mutex(mLock);

        av_packet_unref(&m_Pkt);
        av_packet_move_ref(&m_Pkt, pkt);

        return 0;
    }

private:
    mutable android::base::Lock mLock;
    CStreamInfo                 m_Info;
    AVPacket                    m_Pkt;
    int64_t                     m_nStartTime;
    int64_t                     m_nNextPts;
};

class IrrStreamer {
public:
    IrrStreamer(int w, int h, float framerate) {
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

    ~IrrStreamer() {
        stop();
        av_buffer_pool_uninit(&m_pPool);
    }

    int start(IrrStreamInfo *param) {
        android::base::AutoLock mutex(mLock);

        if (!param || !param->url) {
            av_log(nullptr, AV_LOG_ERROR, "No destination specified.\n");
            return AVERROR(EINVAL);
        }

        if (m_pTrans) {
            av_log(nullptr, AV_LOG_ERROR, "Already running.\n");
            return AVERROR(EINVAL);
        }

        m_pDemux = new IrrVideoDemux(m_nWidth, m_nHeight, m_nPixfmt, m_fFramerate);
        if (!m_pDemux) {
            av_log(nullptr, AV_LOG_ERROR, "Fail to create Demux : Out of memory.\n");
            return AVERROR(ENOMEM);
        }

        m_pTrans   = new CTransCoder(m_pDemux, param->url);
        if (!m_pTrans) {
            av_log(nullptr, AV_LOG_ERROR, "Out of memory.\n");
            delete m_pDemux;
            m_pDemux = nullptr;
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
                    av_log(nullptr, AV_LOG_WARNING, "%s is not a key-value pair.\n", kv.c_str());
                else {
                    av_log(nullptr, AV_LOG_DEBUG, "Find '=' at kv[%lu], key = '%s', value='%s'\n",
                           nKvsep, kv.substr(0, nKvsep).c_str(), kv.substr(nKvsep + 1).c_str());
                    m_pTrans->setOutputProp(kv.substr(0, nKvsep).c_str(), kv.substr(nKvsep + 1).c_str());
                }
            } while (expar.size());
        }

        return m_pTrans->start();
    }

    void stop() {
        android::base::AutoLock mutex(mLock);
        delete m_pTrans;
        m_pTrans = nullptr;
        ///< Demux will be released by CTransCoder's deconstuctor.
        m_pDemux = nullptr;
        m_mPkts.clear();
    }

    int write(const void *data, int size) {
        android::base::AutoLock mutex(mLock);
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

    void *getBuffer() {
        AVBufferRef *pBuf;

        if (!m_pPool)
            return nullptr;

        pBuf = av_buffer_pool_get(m_pPool);
        if (!pBuf) {
            av_log(nullptr, AV_LOG_ERROR, "No free buffer now.\n");
            return nullptr;
        }

        m_mPkts[pBuf->data] = pBuf;
        return pBuf->data;
    }

    int force_key_frame(int force_key_frame) {
        android::base::AutoLock mutex(mLock);
        if(m_pTrans==nullptr) {
            printf("transcoder is not started\n");
            return -1;
        }
        if(force_key_frame) {
            m_pTrans->forceKeyFrame(1);
        }
        return 0;
    }

private:
    IrrVideoDemux *m_pDemux;
    CTransCoder   *m_pTrans;
    AVBufferPool  *m_pPool;
    int            m_nMaxPkts;   ///< Max number of cached frames
    int            m_nCurPkts;
    AVPixelFormat  m_nPixfmt;
    size_t         m_nFrameSize;
    int            m_nWidth, m_nHeight;
    float          m_fFramerate;
    std::map<const void*, AVBufferRef *> m_mPkts;
    mutable android::base::Lock mLock;

    static AVBufferRef* m_BufAlloc(void *opaque, int size) {
        IrrStreamer *pThis = static_cast<IrrStreamer*> (opaque);

        /* Check if the number of packets reaches the limit */
        if (pThis->m_nCurPkts >= pThis->m_nMaxPkts)
            return nullptr;

        return av_buffer_alloc(size);
    }
};

static std::unique_ptr<IrrStreamer> pStreamer = nullptr;

void register_stream_publishment(int w, int h, float framerate) {
    pStreamer.reset(new IrrStreamer(w, h, framerate));
}

int fresh_screen(int w, int h, const void *pixels) {
    if (!pStreamer.get())
        return -EINVAL;

    return pStreamer->write(pixels, w * h * 4);
}

void unregister_stream_publishment() {
    if (!pStreamer.get())
        return;

    pStreamer = nullptr;
}

void *irr_get_buffer(int size) {
    static std::unique_ptr<uint8_t> pBuffer = nullptr;

    if (!pStreamer.get()) {
        if (!pBuffer.get()) {
            pBuffer.reset(new uint8_t[size]);
        }
        return nullptr;
    }

    return pStreamer->getBuffer();
}

int irr_stream_start(struct IrrStreamInfo *stream_info) {
    if (!pStreamer.get())
        return -EINVAL;

    return pStreamer->start(stream_info);
}

void irr_stream_stop() {
    if (pStreamer.get())
        pStreamer->stop();
}

int irr_stream_force_keyframe(int force_key_frame) {
    if (!pStreamer.get())
        return -EINVAL;

    return pStreamer->force_key_frame(force_key_frame);
}
