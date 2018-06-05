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

#ifndef CFFENCODER_H
#define CFFENCODER_H

#include "CEncoder.h"
#include "CTransLog.h"
#include "CStreamInfo.h"

class CFFEncoder : public CEncoder, private CTransLog {
public:
    CFFEncoder(const char *pCodec, CStreamInfo *info, AVDictionary *pDict = nullptr);
    ~CFFEncoder();
    int write(AVFrame *);
    int read(AVPacket *);
    CStreamInfo *getStreamInfo();
    static int GetBestFormat(const char *codec, int format);

private:
    AVCodecContext *m_pEnc;
    bool            m_bInited;
    size_t          m_nFrames;
    AVDictionary   *m_pDict;
    CStreamInfo     m_Info;

private:
    AVCodec *FindEncoder(AVCodecID id);
};

#endif /* CFFENCODER_H */
