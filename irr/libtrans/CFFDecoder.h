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

#ifndef CFFDECODER_H
#define CFFDECODER_H

#include "CTransLog.h"
#include "CDecoder.h"
//#include "CQSVDevice.h"
#include "CVAAPIDevice.h"

class CFFDecoder : public CDecoder, private CTransLog {
public:
    CFFDecoder(CStreamInfo *info, AVDictionary *pDict = nullptr);
    ~CFFDecoder();
    int write(AVPacket *pkt);
    int write(uint8_t *data, int len);
    AVFrame* read(void);
    int getNumFrames(void);
    CStreamInfo* getDecInfo();

private:
    CFFDecoder(const CFFDecoder& orig) = delete;
    CFFDecoder() = delete;
    AVCodec *FindDecoder(AVCodecID id);
    static AVPixelFormat get_format(AVCodecContext *ctx, const AVPixelFormat *pix_fmts);

private:
    AVCodecContext *m_pCodec;
    size_t          m_nFrames;
    CStreamInfo     m_Info;
    bool            m_bInited;
};

#endif /* CFFDECODER_H */
