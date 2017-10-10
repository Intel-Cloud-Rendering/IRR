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

#include "android/utils/gl_cmd_net_format.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

int format_gl_data_command(uint64_t packet_size, uint8_t *packet_data, uint64_t output_buf_size, uint8_t *output_buf)
{
    uint64_t packet_total_size = PACKET_HEAD_LEN + packet_size;
    if (packet_total_size > output_buf_size) {
        fprintf(stderr, "Out of send buffer\n");
        return 0;
    }

    uint8_t packet_major_type = GLPacketType::DATA_PACKET;
    return format_gl_generic_command(packet_major_type, 0, packet_size, packet_data, output_buf_size, output_buf);
}

int format_gl_ctrl_command(GLCtrlType gl_ctrl_type, uint64_t output_buf_size, uint8_t *output_buf)
{
    uint64_t packet_size = PACKET_HEAD_LEN;
    if (packet_size > output_buf_size) {
        fprintf(stderr, "Out of send buffer\n");
        return 0;
    }

    int offset = 0;
    uint8_t packet_major_type = GLPacketType::CTRL_PACKET;
    *output_buf = packet_major_type;
    offset += sizeof(uint8_t);

    *((uint8_t *)(output_buf + offset)) = (uint8_t)gl_ctrl_type;
    offset += sizeof(uint8_t);

    *((uint64_t *)(output_buf + offset)) = 0;
    offset += sizeof(uint64_t);

    return offset;
}

int format_gl_generic_command(
    uint8_t major,
    uint8_t minor,
    uint64_t packet_size,
    uint8_t *packet_data,
    uint64_t output_buf_size,
    uint8_t *output_buf)
{
    uint64_t packet_total_size = PACKET_HEAD_LEN + packet_size;
    if (packet_total_size > output_buf_size) {
        fprintf(stderr, "Out of send buffer\n");
        return 0;
    }

    int offset = 0;
    *output_buf = major;
    offset += sizeof(uint8_t);

    *(output_buf + offset) = minor;
    offset += sizeof(uint8_t);

    *((uint64_t *)(output_buf + offset)) = packet_size;
    offset += sizeof(uint64_t);

    assert(offset == PACKET_HEAD_LEN);
    memcpy(output_buf + offset, packet_data, packet_size);
    offset += packet_size;

    return offset;
}

