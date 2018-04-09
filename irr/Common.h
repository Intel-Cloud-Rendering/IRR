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

#include <stdio.h>
#include <stdint.h>

#ifndef __COMMON_H__
#define __COMMON_H__

#define IRR_DEBUG_LEVEL 3

namespace irr {

typedef char      irr_bool_t;
typedef int       irr_int_t;
typedef int64_t   irr_disksize_t;
typedef char*     irr_string_t;
typedef double    irr_double_t;
typedef FILE      irr_file_t;

/* these macros are used to define the fields of IrrHwConfig
 *  *  * declared below
 *   *   */
#define   IRRCFG_BOOL(n,s,d,a,t)       irr_bool_t      n;
#define   IRRCFG_INT(n,s,d,a,t)        irr_int_t       n;
#define   IRRCFG_STRING(n,s,d,a,t)     irr_string_t    n;
#define   IRRCFG_DOUBLE(n,s,d,a,t)     irr_double_t    n;
#define   IRRCFG_DISKSIZE(n,s,d,a,t)   irr_disksize_t  n;

// Usage: Define IRR_DEBUG_LEVEL before including this header to
//        select the behaviour of the D() and DD() macros.
//

#if defined(IRR_DEBUG_LEVEL) && IRR_DEBUG_LEVEL > 0
#include <stdio.h>
#define D(...)  (printf("%s:%d:%s: ", __FILE__, __LINE__, \
                                    __func__), printf(__VA_ARGS__), printf("\n"), fflush(stdout))
#else
#define D(...)  (void)0
#endif // IRR_DEBUG_LEVEL > 0

#if defined(IRR_DEBUG_LEVEL) && IRR_DEBUG_LEVEL > 1
#define DD(...) D(__VA_ARGS__)
#else
#define DD(...) (void)0
#endif // IRR_DEBUG_LEVEL > 1

}

#endif //__COMMON_H__
