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

#include <fcntl.h>
#include <memory>
#include <sys/un.h>
#include <unistd.h>

#include "Dump.h"

namespace irr {

static void on_dump_timeout(void* opaque, Looper::Timer* timer) {
    const auto dump = static_cast<Dump*>(opaque);
    if (dump) {
        dump->stopDumpCmd();
    }
}

bool Dump::isDump(void)
{
    return (!m_stop);
}

void Dump::tryFrame(struct DumpFrameInfo *fr_info)
{
    int fd = -1;

    {
        std::lock_guard<std::mutex> guard(m_mutex);
        if (!m_stop) {
            fd = open(m_fname.c_str(), O_WRONLY|O_CREAT|O_APPEND, 0666);
            if (fd > 0) {
                m_count ++;
                if ((m_frame_total > 0) && (m_count >= m_frame_total)) { // if stop by frame count
                    m_stop = true;
                }
            } else {
                fprintf(stderr, "Warning: stream dump failed to open file %s\n", m_fname.c_str());
            }
        }
    }

    if (fd > 0) {
        D("dump frame to file %d,%d,%d \n", fr_info->width, fr_info->height, fr_info->ydir);
        int sz = 0;
        switch (fr_info->format) {
            case GL_LUMINANCE:
                sz = fr_info->width * fr_info->height * 3 / 2;
                break;
            case GL_RGBA:
            default:
                sz = fr_info->width * fr_info->height * 4;
                break;
        }
        write(fd, fr_info->pixels, sz);
        close(fd);
    }

    return;
}

int Dump::startDumpCmd(const DumpInfo& dump_info)
{
    if (!m_stop) {
        return -1;
    }

    std::lock_guard<std::mutex> guard(m_mutex);

    // set new values
    std::stringstream ss;
    ss << m_dump_dir << "/" << dump_info.filename << "_" << dump_info.serial_no << m_file_ext;
    m_fname = ss.str();

    int fd = open(m_fname.c_str(), O_RDONLY|O_CREAT|O_TRUNC, 0666);
    if (fd <= 0) {
        fprintf(stderr, "Warning: stream dump failed to open file %s for read.\n", m_fname.c_str());
        return -1;
    }
    close(fd);

    if (dump_info.__isset.dur_s) { // measure by duration
        m_timer.reset(m_looper->createTimer(on_dump_timeout, this));
        const Looper::Duration dl = 1000 * dump_info.dur_s;
        m_timer->startRelative(dl);

    } else if (dump_info.__isset.frame_total) { // measure by frame count
        m_frame_total = dump_info.frame_total;

    } // else {}  continuing dump, without stop condition

    m_stop = false;

    return 0;
}

void Dump::stopDumpCmd()
{
    std::lock_guard<std::mutex> guard(m_mutex);
    m_stop = true;

    if (m_timer.get()) {
        m_timer->stop();
        m_timer.reset();
    }

    // clear frame count
    m_frame_total = 0;
    m_count = 0;
}

const char* Dump::RGBA_EXT = ".rgba";
const char* Dump::YUV_EXT = ".yuv";

static std::unique_ptr<irr::Dump> sDump;

// init dump instance only if DUMP_DIR env is set.
int init_dump(const char* dump_dir, const char* port_num, int format, Looper* looper) {
    sDump.reset(new irr::Dump(dump_dir, port_num, format, looper));
    if (!sDump->initDump()) {
        return 0;
    }
    sDump.reset();
    return -1;
}

// send dump cmd
int start_dump_cmd(const DumpInfo& dump_info) {
    if (sDump.get()) {
        return sDump->startDumpCmd(dump_info);
    }
    return -1;
}

int stop_dump_cmd() {
    if (sDump.get()) {
        sDump->stopDumpCmd();
    }
    return 0;
}

bool is_dump() {
    if (sDump.get()) {
        return sDump->isDump();
    }
    return false;
}

// dump one frame
int dump_1frame(struct DumpFrameInfo *fr_info) {
    if (sDump.get()) {
        sDump->tryFrame(fr_info);
    }
    return 0;
}

}
