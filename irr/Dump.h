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

#ifndef __DUMP_H__
#define __DUMP_H__

#include <mutex>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <GLES/gl.h>

#include "android/base/async/Looper.h"
#include "Common.h"
#include "rpc-thrift/generated/main_types.h"

extern "C" {

namespace irr{

using android::base::Looper;
using IntelCloudRendering::DumpInfo;

struct DumpFrameInfo {
    int width;
    int height;
    int ydir;
    int format;
    int type;
    unsigned char* pixels;
};

class Dump{
public:
    Dump(const char* dump_dir, const char* port_num, int format, Looper* looper)
        :m_looper(looper),
        m_frame_total(0),
        m_count(0),
        m_timer(),
        m_stop(true)
    {
        std::stringstream ss;
        ss << dump_dir << "/" << port_num;
        m_dump_dir = ss.str();

        switch (format) {
            // for dump yuv files, if use libyuv lib do rgba-> yuv dump
            case GL_LUMINANCE:
                m_file_ext = YUV_EXT;
                break;
            case GL_RGBA:
            default:
                m_file_ext = RGBA_EXT;
                break;
        }
    }

    int initDump()
    {
        if (!m_dump_dir.empty()) {
            struct stat st = {0};
            if (stat(m_dump_dir.c_str(), &st) == -1) {
                if (mkdir(m_dump_dir.c_str(), 0775) == -1) {
                    fprintf(stderr, "Warning: stream dump failed to mkdir %s\n", m_dump_dir.c_str());
                    return -1;
                }
            }
            return 0;
        }
        return -1;
    }

    virtual ~Dump() {
        stopDumpCmd();
    }

    bool isDump(void);

    void tryFrame(struct DumpFrameInfo *fr_info);

    int startDumpCmd(const DumpInfo& dump_info);

    void stopDumpCmd();

private:
    std::string m_dump_dir;  // dump dir name
    std::string m_file_ext; // dump file extension
    Looper* m_looper;

    std::string m_fname; // a dynamically allocated place to hold "m_dump_dir", "file_name", "serial_no" and "m_file_ext"
    int m_frame_total; // store dump total frames
    int m_count; // dumped frame counter, set to 0 each time new cmd is coming.
    std::unique_ptr<Looper::Timer> m_timer; //store dump timer
    bool m_stop; // stop dump flag

    std::mutex m_mutex;

    static const char* RGBA_EXT;
    static const char* YUV_EXT;
};

// init dump instance only if DUMP_DIR env is set.
int init_dump(const char* dump_dir, const char* port_num, int format, Looper* looper);

// start dump cmd
int start_dump_cmd(const DumpInfo& dump_info);

// stop dump cmd
int stop_dump_cmd();

// check dump status
bool is_dump();

// dump one frame
int dump_1frame(struct DumpFrameInfo *fr_info);

}
}
#endif //__DUMP_H__
