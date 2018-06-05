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

extern "C" {
#include <libavdevice/avdevice.h>
#include <libavfilter/avfilter.h>
}

#include "CQSVDevice.h"
#include "version.h"

CQSVDevice *CQSVDevice::m_pQsvDev = nullptr;
static CQSVDevice::Maintainer maintainer;

CQSVDevice::CQSVDevice() : CTransLog(__func__), m_pDev(nullptr) {
    AVDictionary *dict = nullptr;
    int ret = 0;
    const char *dev_dri;

    Info("Libtrans version %s\nCopyright (C) 2017 Intel\n", LIBTRANS_VERSION);
    if ((dev_dri = getenv("MFX_DEVICE")) != nullptr) {
        ret = av_dict_set(&dict, "child_device", dev_dri, 0);
        if (ret < 0) {
            Error("Unable to create a dict. Msg: %s.\n", ErrToStr(ret).c_str());
            return;
        }
    }

    ret = av_hwdevice_ctx_create(&m_pDev, AV_HWDEVICE_TYPE_QSV, "qsvdev", dict, 0);
    if (ret < 0) {
        av_dict_free(&dict);
        Error("Unable to create a valid QSV device. Msg: %s.\n", ErrToStr(ret).c_str());
        return;
    }

    if (dict) {
        av_dict_free(&dict);
    }
}

CQSVDevice::~CQSVDevice() {
    av_buffer_unref(&m_pDev);
}

CQSVDevice *CQSVDevice::getInstance() {
    if (!m_pQsvDev)
        m_pQsvDev = new CQSVDevice();
    return m_pQsvDev;
}

AVBufferRef *CQSVDevice::getQsvDev() {
    return m_pDev;
}

CQSVDevice::Maintainer::Maintainer() {
    avcodec_register_all();
    avdevice_register_all();
    avfilter_register_all();
    av_register_all();
    avformat_network_init();
    CTransLog::SetLogLevel(CTransLog::LL_INFO);

    getInstance();
}

CQSVDevice::Maintainer::~Maintainer() {
    if (m_pQsvDev) {
        delete m_pQsvDev;
        m_pQsvDev = nullptr;
    }
}

