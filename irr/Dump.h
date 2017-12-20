
#ifndef __DUMP_H__
#define __DUMP_H__

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>

#include "Common.h"
#include "android/opengles.h"

extern "C" {

namespace irr{


class Dump{
public:
    Dump(const char* dump_env, const char* file_name)
        :m_dump_env(dump_env),
        m_file_name(file_name),
        m_dump_dir(NULL),
        m_fp(NULL),
        m_fname(NULL),
        m_count(0)
    {}

    virtual ~Dump() {
        if (m_fp != NULL) {
            fclose(m_fp);
            m_fp = nullptr;
        }
        if (m_fname != NULL) {
            delete[] m_fname;
            m_fname = nullptr;
        }
    }

    bool isDump(void);

    void tryFrame(int width, int height, int ydir,
              int format, int type, unsigned char* pixels);

protected:
    FILE* getFP(void) const { return m_fp;}

private:
    const char* m_dump_env;
    const char* m_file_name;
    char* m_dump_dir;
    FILE* m_fp;
    char* m_fname;
    int m_count;
};

using DumpPtr = std::shared_ptr<Dump>;

}
}
#endif //__DUMP_H__
