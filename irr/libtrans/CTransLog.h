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

#ifndef CTRANSLOG_H
#define CTRANSLOG_H

#include <iostream>

class CTransLog {
public:
    enum LogLevel {
        LL_DEBUG,
        LL_INFO,
        LL_WARN,
        LL_ERROR,
        LL_NONE,
        LL_NUM
    };

    CTransLog(const char *name);
    ~CTransLog();
    static void SetLogLevel(LogLevel level);
    std::string ErrToStr(int ret);
    std::string TsToStr(int64_t ts);
    std::string TsToTimeStr(int64_t ts, int num, int den);
    void Debug(const char *format, ...);
    void Info(const char *format, ...);
    void Warn(const char *format, ...);
    void Error(const char *format, ...);

private:
    void *m_pClass;
};

#endif /* CTRANSLOG_H */

