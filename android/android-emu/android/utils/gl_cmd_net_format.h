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

#include <stdint.h>

ANDROID_BEGIN_HEADER

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

ANDROID_END_HEADER
