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

#ifndef CVAAPIDEVICE_H
#define CVAAPIDEVICE_H

extern "C" {
#include <libavutil/hwcontext.h>
#include <libavutil/hwcontext_vaapi.h>
}

#include <memory>
#include "CTransLog.h"

class CVAAPIDevice : private CTransLog {
public:
    static CVAAPIDevice *getInstance();
    AVBufferRef *getVaapiDev();

    class Maintainer {
    public:
        Maintainer();
        ~Maintainer();
    };

private:
    CVAAPIDevice();
    CVAAPIDevice(const CVAAPIDevice& orig);
    CVAAPIDevice& operator=(CVAAPIDevice &orig);
    ~CVAAPIDevice();

private:
    AVBufferRef *m_pDev;
    static CVAAPIDevice* m_pVaapiDev;
};

#endif /* CVAAPIDEVICE_H */

