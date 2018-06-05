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

#include <stdarg.h>
extern "C" {
#include <libavutil/avutil.h>
#include <libavutil/timestamp.h>
}
#include "version.h"
#include "CTransLog.h"

using namespace std;

CTransLog::CTransLog(const char *name) : m_pClass(nullptr) {
    m_pClass = av_mallocz(sizeof(AVClass));
    ((AVClass *)m_pClass)->class_name = name;
    ((AVClass *)m_pClass)->item_name  = av_default_item_name;
    ((AVClass *)m_pClass)->version    = LIBTRANS_VERSION_INT;
}

CTransLog::~CTransLog() {
    av_freep(&m_pClass);
}

void CTransLog::SetLogLevel(LogLevel level) {
    int avlevel = AV_LOG_QUIET;

    switch (level) {
        case LL_DEBUG: avlevel = AV_LOG_DEBUG; break;
        case LL_INFO:  avlevel = AV_LOG_INFO; break;
        case LL_WARN:  avlevel = AV_LOG_WARNING; break;
        case LL_ERROR: avlevel = AV_LOG_ERROR; break;
        default: break;
    }

    av_log_set_level(avlevel);
}

string CTransLog::ErrToStr(int ret) {
    char buf[AV_ERROR_MAX_STRING_SIZE] = {0};
    return av_make_error_string(buf, AV_ERROR_MAX_STRING_SIZE, ret);
}

string CTransLog::TsToStr(int64_t ts) {
    char buf[AV_ERROR_MAX_STRING_SIZE] = {0};
    return av_ts_make_string(buf, ts);
}

string CTransLog::TsToTimeStr(int64_t ts, int num, int den) {
    char buf[AV_ERROR_MAX_STRING_SIZE] = {0};
    AVRational tb = { num, den };
    return av_ts_make_time_string(buf, ts, &tb);
}

void CTransLog::Debug(const char *format, ...) {
    va_list list;
    va_start(list, format);
    av_vlog(&m_pClass, AV_LOG_DEBUG, format, list);
    va_end(list);
}

void CTransLog::Info(const char *format, ...) {
    va_list list;
    va_start(list, format);
    av_vlog(&m_pClass, AV_LOG_INFO, format, list);
    va_end(list);
}

void CTransLog::Warn(const char *format, ...) {
    va_list list;
    va_start(list, format);
    av_vlog(&m_pClass, AV_LOG_WARNING, format, list);
    va_end(list);
}

void CTransLog::Error(const char *format, ...) {
    va_list list;
    va_start(list, format);
    av_vlog(&m_pClass, AV_LOG_ERROR, format, list);
    va_end(list);
}
