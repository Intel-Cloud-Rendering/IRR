
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
