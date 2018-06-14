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

#ifndef IRRSTREAMER_H
#define IRRSTREAMER_H

#include <map>
#include <inttypes.h>
#include <stddef.h>
#include "libtrans/CIrrVideoDemux.h"
#include "libtrans/CTransCoder.h"
#include "libtrans/CTransLog.h"

class IrrStreamer : public CTransLog {
public:
    struct IrrStreamInfo {
        /* Output-only parameters */
        int bitrate;               ///< Encoder bitrate, default 1M
        int gop_size;              ///< Group Of Picture size, default 50
        const char *codec;         ///< Encoder codec, e.x. h264_qsv; may be null
        const char *format;        ///< Mux format, e.x. flv; null as auto
        const char *url;           ///< Output url.
        int low_power;             ///< Enable low-power mode, default not.
        const char *res;           ///< Encoding resolution.
        const char *framerate;     ///< Encoding framerate
        const char *exp_vid_param; ///< Extra encoding/muxer parameters passed to libtrans/FFmpeg
        struct CallBackTable {     ///< Callback function tables
            void *opaque;          ///< Used by callback functions
            int (*cbOpen) (void */*opaque*/, int /*w*/, int /*h*/, float /*frame_rate*/);
            /* Synchronous write callback*/
            int (*cbWrite) (void */*opaque*/, uint8_t */*data*/, size_t/*size*/);
            void (*cbClose) (void */*opaque*/);
        } cb_params;
    };

    IrrStreamer(int w, int h, float framerate);
    ~IrrStreamer();

    int   start(IrrStreamInfo *param);
    void  stop();
    int   write(const void *data, int size);
    void *getBuffer();
    int   force_key_frame(int force_key_frame);

private:
    CIrrVideoDemux*m_pDemux;
    CTransCoder   *m_pTrans;
    AVBufferPool  *m_pPool;
    int            m_nMaxPkts;   ///< Max number of cached frames
    int            m_nCurPkts;
    AVPixelFormat  m_nPixfmt;
    size_t         m_nFrameSize;
    int            m_nWidth, m_nHeight;
    float          m_fFramerate;
    std::map<const void*, AVBufferRef *> m_mPkts;
    std::mutex     m_Lock;

    static AVBufferRef* m_BufAlloc(void *opaque, int size);
};

#endif /* IRRSTREAMER_H */
