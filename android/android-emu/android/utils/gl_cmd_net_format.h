// Copyright 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#pragma once

#include "android/utils/compiler.h"
#include "android/utils/system.h"

#include <stdint.h>
#include <stdio.h>
#include <assert.h>

ANDROID_BEGIN_HEADER

#define DEBUG 0

#if DEBUG >= 1
#define D(...) printf(__VA_ARGS__), printf("\n"), fflush(stdout)
#else
#define D(...) ((void)0)
#endif

#if DEBUG >= 2
#define DD(...) printf(__VA_ARGS__), printf("\n"), fflush(stdout)
#else
#define DD(...) ((void)0)
#endif

#if DEBUG >= 3
#define DDD(...) printf(__VA_ARGS__), printf("\n"), fflush(stdout)
#define AutoLog() AutoLogger autoLogger(__func__, this)
#else
#define DDD(...) ((void)0)
#define AutoLog() ((void)0)
#endif

typedef enum {
    CTRL_PACKET = 0,
    DATA_PACKET
} GLPacketType;

typedef enum {
    CLOSE_CTRL = 0,
    POLL_CTRL,
    SET_STATE_CTRL
} GLCtrlType;

typedef struct _GLCmdPacketHead {
    uint8_t major_type;
    uint8_t minor_type;
    uint64_t packet_size;
} GLCmdPacketHead;

#define PACKET_MAJOR_TYPE_LEN (1)
#define PACKET_MINOR_TYPE_LEN (1)
#define PACKET_SIZE_LEN       (8)

#define PACKET_HEAD_LEN (PACKET_MAJOR_TYPE_LEN + PACKET_MINOR_TYPE_LEN + PACKET_SIZE_LEN)

class AutoLogger {
public:
    AutoLogger(const char *name, void *id) {
        mTid = android_get_thread_id();
        mId = id;
        assert(strlen(name) < sizeof(mFuncName));
        strcpy(mFuncName, name);
        printf("[DEBUG][%p][0x%" ANDROID_THREADID_FMT "][%s <<<<<]\n", mId, mTid, mFuncName);
    };
    ~AutoLogger() {
        printf("[DEBUG][%p][0x%" ANDROID_THREADID_FMT "][%s >>>>>]\n", mId, mTid, mFuncName);
    };
private:
    char mFuncName[512] = {0};
    void *mId;
    android_thread_id_t mTid;
};

int format_gl_data_command(
    uint64_t packet_size,
    uint8_t *packet_data,
    uint64_t output_buf_size,
    uint8_t *output_buf);

int format_gl_ctrl_command(
    GLCtrlType gl_ctrl_type,
    uint64_t output_buf_size,
    uint8_t *output_buf);

int format_gl_generic_command(
    uint8_t major,
    uint8_t minor,
    uint64_t packet_size,
    uint8_t *packet_data,
    uint64_t output_buf_size,
    uint8_t *output_buf);

FILE *openDumpFile(const char *name);
void writeToDumpFile(FILE *file, uint8_t *data, uint32_t dataLen);
void closeDumpFile(FILE *file);

ANDROID_END_HEADER
