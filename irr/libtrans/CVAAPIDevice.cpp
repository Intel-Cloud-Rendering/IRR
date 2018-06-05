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

#include "CVAAPIDevice.h"
#include "version.h"

CVAAPIDevice *CVAAPIDevice::m_pVaapiDev = nullptr;
static CVAAPIDevice::Maintainer maintainer;

CVAAPIDevice::CVAAPIDevice() : CTransLog(__func__) {
    int ret = 0;
    const char *dev_dri;

    Info("Libtrans version %s\nCopyright (C) 2017 Intel\n", LIBTRANS_VERSION);

    dev_dri = getenv("VAAPI_DEVICE");
    if (!dev_dri)
        dev_dri = "/dev/dri/renderD128";

    ret = av_hwdevice_ctx_create(&m_pDev, AV_HWDEVICE_TYPE_VAAPI, dev_dri, nullptr, 0);
    if (ret < 0) {
        Error("Unable to create a valid VAAPI device. Msg: %s.\n", ErrToStr(ret).c_str());
        return;
    }
}

CVAAPIDevice::~CVAAPIDevice() {
    av_buffer_unref(&m_pDev);
}

CVAAPIDevice *CVAAPIDevice::getInstance() {
    if (!m_pVaapiDev)
        m_pVaapiDev = new CVAAPIDevice();
    return m_pVaapiDev;
}

AVBufferRef *CVAAPIDevice::getVaapiDev() {
    return m_pDev;
}

CVAAPIDevice::Maintainer::Maintainer() {
    avcodec_register_all();
    avdevice_register_all();
    avfilter_register_all();
    av_register_all();
    avformat_network_init();
    CTransLog::SetLogLevel(CTransLog::LL_INFO);

    getInstance();
}

CVAAPIDevice::Maintainer::~Maintainer() {
    if (m_pVaapiDev) {
        delete m_pVaapiDev;
        m_pVaapiDev = nullptr;
    }
}
