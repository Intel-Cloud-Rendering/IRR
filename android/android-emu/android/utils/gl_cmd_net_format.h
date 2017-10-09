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
    POLL_CTRL
} GLCtrlType;

#define PACKET_TYPE_LEN (1)
#define PACKET_SIZE_LEN (8)

int format_gl_data_command(GLPacketType gl_packet_type, uint64_t packet_size, uint8_t *packet_data, uint64_t output_buf_size, uint8_t *output_buf);
int format_gl_ctrl_command(GLCtrlType gl_ctrl_type, uint64_t output_buf_size, uint8_t *output_buf);

ANDROID_END_HEADER
