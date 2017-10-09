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

#include <string.h>

int format_gl_data_command(GLPacketType gl_packet_type, uint64_t packet_size, uint8_t *packet_data, uint64_t output_buf_size, uint8_t *output_buf)
{
    if (packet_size > output_buf_size) {
        return 0;
    }
    
    int offset = 0;
    uint8_t packet_type = gl_packet_type;
    *output_buf = packet_type;
    offset += sizeof(uint8_t);
    
    *((uint64_t *)(output_buf + offset)) = packet_size;
    offset += sizeof(uint64_t);

    memcpy(output_buf + offset, packet_data, packet_size);
    offset += packet_size;

    return offset;
}


int format_gl_ctrl_command(GLCtrlType gl_ctrl_type, uint64_t output_buf_size, uint8_t *output_buf)
{
    uint64_t packet_size = sizeof(uint8_t) + sizeof(uint64_t) + sizeof(uint8_t);
    if (packet_size > output_buf_size) {
        return 0;
    }
    
    int offset = 0;
    uint8_t packet_type = CTRL_PACKET;
    *output_buf = packet_type;
    offset += sizeof(uint8_t);
   
    *((uint64_t *)(output_buf + offset)) = sizeof(uint8_t);
    offset += sizeof(uint64_t);

    uint8_t ctrl_type = gl_ctrl_type;
    *((uint8_t *)(output_buf + offset)) = ctrl_type;
    offset += sizeof(uint8_t);

    return offset;
}
