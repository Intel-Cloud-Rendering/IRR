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

#ifndef CCALLBACKMUX_H
#define CCALLBACKMUX_H

#include "CMux.h"
#include "CTransLog.h"

typedef int (*cbOpen) (void *, int /*w*/, int /*h*/, float /*framerate*/);
typedef int (*cbWrite) (void *, uint8_t *, size_t);
typedef void (*cbClose) (void *);

class CCallbackMux : public CMux, private CTransLog {
public:
    CCallbackMux(void *opaque, cbOpen pOpen, cbWrite pWrite, cbClose pClose);
    ~CCallbackMux();
    int addStream(int, CStreamInfo *);
    int write(AVPacket *);

private:
    void       *m_Opaque;
    cbOpen      m_pOpen;
    cbWrite     m_pWrite;
    cbClose     m_pClose;
    bool        m_bInited;
};

#endif /* CCALLBACKMUX_H */

