
extern "C" {
#include "libavutil/time.h"
}
#include "android/cmdline-option.h"
#include "android/base/synchronization/Lock.h"
#include "CTransCoder.h"

class CMyDemux : public CDemux {
public:
    CMyDemux(int w, int h, int format) {
        m_Info.m_pCodecPars->codec_type = AVMEDIA_TYPE_VIDEO;
        m_Info.m_pCodecPars->codec_id   = AV_CODEC_ID_RAWVIDEO;
        m_Info.m_pCodecPars->format     = format;
        m_Info.m_pCodecPars->width      = w;
        m_Info.m_pCodecPars->height     = h;
        m_Info.m_rFrameRate             = (AVRational) {30, 1};
        m_Info.m_rTimeBase              = AV_TIME_BASE_Q;
        m_nStartTime                    = av_gettime_relative();
        m_nNextPts                      = 0;
        av_init_packet(&m_Pkt);
    }

    ~CMyDemux() {
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

    int sendPacket(const void *data, size_t len) {
        android::base::AutoLock mutex(mLock);

        av_packet_unref(&m_Pkt);
        m_Pkt.buf  = av_buffer_alloc(len);
        m_Pkt.data = m_Pkt.buf->data;
        m_Pkt.size = len;
        memcpy(m_Pkt.data, data, len);

        return 0;
    }

private:
    mutable android::base::Lock mLock;
    CStreamInfo                 m_Info;
    AVPacket                    m_Pkt;
    int64_t                     m_nStartTime;
    int64_t                     m_nNextPts;
};

static CMyDemux    *myDemux = nullptr;
static CTransCoder *myTrans = nullptr;

int register_stream_publishment() {
    return 0;
}

int fresh_screen(int w, int h, const void *pixels) {
    int ret;

    if (!myDemux) {
        myDemux = new CMyDemux(w, h, AV_PIX_FMT_RGBA);
        if (!myDemux)
            return -1;
    }

    ret = myDemux->sendPacket(pixels, w * h * 4);
    if (ret < 0)
        return ret;

    if (!myTrans) {
        if (android_cmdLineOptions->url)
            myTrans = new CTransCoder(dynamic_cast<CDemux*>(myDemux), android_cmdLineOptions->url, "h264");
        if (!myTrans)
            return -2;
        myTrans->setOutputProp("b", android_cmdLineOptions->b ? android_cmdLineOptions->b : "2M");  ///< Bitrate
        myTrans->setOutputProp("c", android_cmdLineOptions->codec ? android_cmdLineOptions->codec : "h264_qsv");///< Codec
        myTrans->setOutputProp("g", "50");  ///< GOP size
        myTrans->setOutputProp("r", android_cmdLineOptions->fr ? android_cmdLineOptions->fr : "25");  ///< Framerate
        if (android_cmdLineOptions->res)
            myTrans->setOutputProp("s", android_cmdLineOptions->res); ///< Resolution
        myTrans->setOutputProp("buffer_size", "2632");

        //av_log_set_level(AV_LOG_DEBUG);
        ret = myTrans->start();
    }

    return ret;
}

void unregister_stream_publishment() {
    if (myTrans) {
        myTrans->stop();
        delete myTrans;
        myTrans = nullptr;
    }
}
