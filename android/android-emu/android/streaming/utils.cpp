
#include "utils.h"
#include <map>
#include <memory>
#include "android/base/synchronization/Lock.h"
#include "CTransCoder.h"
ANDROID_BEGIN_HEADER
#include "libavutil/time.h"
#include "libavutil/imgutils.h"
ANDROID_END_HEADER

class IrrVideoDemux : public CDemux {
public:
    IrrVideoDemux(int w, int h, int format, int framerate) {
        m_Info.m_pCodecPars->codec_type = AVMEDIA_TYPE_VIDEO;
        m_Info.m_pCodecPars->codec_id   = AV_CODEC_ID_RAWVIDEO;
        m_Info.m_pCodecPars->format     = format;
        m_Info.m_pCodecPars->width      = w;
        m_Info.m_pCodecPars->height     = h;
        m_Info.m_rFrameRate             = (AVRational) {framerate, 1};
        m_Info.m_rTimeBase              = AV_TIME_BASE_Q;
        m_nStartTime                    = av_gettime_relative();
        m_nNextPts                      = 0;
        av_init_packet(&m_Pkt);
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
    IrrStreamer() {
        m_nMaxPkts = 5;
        m_nCurPkts = 0;
        m_pDemux   = nullptr;
        m_pTrans   = nullptr;
        m_pPool    = nullptr;
        m_nPixfmt  = AV_PIX_FMT_RGBA;
    }

    ~IrrStreamer() {
        stop();
    }

    int start(IrrStreamInfo *param) {
        int size;
        void *data;

        if (!param || !param->url) {
            av_log(nullptr, AV_LOG_ERROR, "No destination specified.\n");
            return AVERROR(EINVAL);
        }

        size       = av_image_get_buffer_size(m_nPixfmt, param->in.w, param->in.h, 32);
        m_pDemux   = new IrrVideoDemux(param->in.w, param->in.h, m_nPixfmt, param->in.framerate);
        m_pTrans   = new CTransCoder(m_pDemux, param->url);
        if (!m_pTrans || !m_pDemux) {
            av_log(nullptr, AV_LOG_ERROR, "Out of memory.\n");
            return AVERROR(ENOMEM);
        }

        m_pPool = av_buffer_pool_init2(size, this, m_BufAlloc, nullptr);
        if (!m_pPool) {
            av_log(nullptr, AV_LOG_ERROR, "Out of memory.\n");
            return AVERROR(ENOMEM);
        }

        /*
         * Make a fake packet here, which is a black frame.
         * Aim to make libtrans work.
         */
        data = getBuffer();
        if (!data) {
            av_log(nullptr, AV_LOG_ERROR, "fail to query a buffer.\n");
            return AVERROR(ENOMEM);
        }

        write(data, size);

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

        if (param->out.framerate)
            SETOPTINT("r", param->out.framerate); ///< Framerate

        if (param->low_power)
            m_pTrans->setOutputProp("low_power", "1");

        if (param->out.w && param->out.h) {
            char buf[32];
            snprintf(buf, sizeof(buf), "%dx%d", param->out.w, param->out.h);
            m_pTrans->setOutputProp("s", buf);
        }

        if (param->gop_size)
            SETOPTINT("g", param->gop_size); ///< GOP size

        return m_pTrans->start();
    }

    void stop() {
        delete m_pTrans;
        m_pTrans = nullptr;
        m_mPkts.clear();
        av_buffer_pool_uninit(&m_pPool);
    }

    int write(const void *data, int size) {
        auto it = m_mPkts.find(data);

        if (!m_pTrans)
            return AVERROR(EINVAL);

        if (it != m_mPkts.end()) {
            AVPacket pkt;

            av_init_packet(&pkt);
            pkt.buf  = it->second;
            pkt.data = pkt.buf->data;
            pkt.size = pkt.buf->size;
            pkt.stream_index = 0;
            m_mPkts.erase(it);
            return m_pDemux->sendPacket(&pkt);
        }

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

private:
    IrrVideoDemux *m_pDemux;
    CTransCoder   *m_pTrans;
    AVBufferPool  *m_pPool;
    int            m_nMaxPkts;   ///< Max number of cached frames
    int            m_nCurPkts;
    AVPixelFormat  m_nPixfmt;
    std::map<const void*, AVBufferRef *> m_mPkts;

    static AVBufferRef* m_BufAlloc(void *opaque, int size) {
        IrrStreamer *pThis = static_cast<IrrStreamer*> (opaque);

        /* Check if the number of packets reaches the limit */
        if (pThis->m_nCurPkts >= pThis->m_nMaxPkts)
            return nullptr;

        return av_buffer_alloc(size);
    }
};

static IrrStreamer streamer;

int register_stream_publishment(struct IrrStreamInfo *param) {
    streamer.stop();
    return streamer.start(param);
}

int fresh_screen(int w, int h, const void *pixels) {
    return streamer.write(pixels, w * h * 4);
}

void unregister_stream_publishment() {
    streamer.stop();
}

void *irr_get_buffer(int size) {
    return streamer.getBuffer();
}
