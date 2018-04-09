// Copyright 2018 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "Dump.h"

namespace irr {


bool Dump::isDump(void)
{
    if (!m_fp){
        m_dump_dir = getenv(m_dump_env);
        if (m_dump_dir) {
            size_t bsize = strlen(m_dump_dir) + 32;
            m_fname = new char[bsize];
            snprintf(m_fname, bsize, "%s/stream_%s.argb", m_dump_dir, m_file_name);
            m_fp = fopen(m_fname, "ab");
            if (!m_fp) {
                fprintf(stderr, "Warning: stream dump failed to open file %s\n", m_fname);
            }
        }
    }
    return (m_fp!=NULL);
}


void Dump::tryFrame(int width, int height, int ydir,
              int format, int type, unsigned char* pixels)
{
    if (isDump()){
        D("dump frame to file %d,%d,%d \n", width, height, ydir);
        fwrite(pixels, sizeof(int), width*height, getFP());
        fflush(getFP());
    }
    return;
}

}
