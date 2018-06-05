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

#ifndef CQSVDEVICE_H
#define CQSVDEVICE_H

extern "C" {
#include <libavutil/hwcontext.h>
#include <libavutil/hwcontext_qsv.h>
}

#include <memory>
#include "CTransLog.h"

class CQSVDevice : private CTransLog {
public:
    static CQSVDevice *getInstance();
    AVBufferRef *getQsvDev();

    class Maintainer {
    public:
        Maintainer();
        ~Maintainer();
    };

private:
    CQSVDevice();
    CQSVDevice(const CQSVDevice& orig);
    CQSVDevice& operator=(CQSVDevice &orig);
    ~CQSVDevice();

private:
    AVBufferRef *m_pDev;
    static CQSVDevice* m_pQsvDev;
};

#endif /* CQSVDEVICE_H */

