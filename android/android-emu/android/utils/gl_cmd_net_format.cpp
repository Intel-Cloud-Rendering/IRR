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

int format_gl_data_command(int32_t packet_size, uint8_t *packet_data, int32_t output_buf_size, uint8_t *output_buf)
{
    int32_t packet_total_size = PACKET_HEAD_LEN + packet_size;
    if (packet_total_size > output_buf_size) {
        fprintf(stderr, "Out of send buffer\n");
        return 0;
    }

    GLNetworkPacketType packet_major_type = GLNetworkPacketType::DATA_PACKET;
    return format_gl_generic_command(packet_major_type, packet_size, packet_data, output_buf_size, output_buf);
}

int format_gl_ctrl_command(GLNetworkPacketType gl_ctrl_type, int32_t output_buf_size, uint8_t *output_buf)
{
    int32_t packet_size = PACKET_HEAD_LEN;
    if (packet_size > output_buf_size) {
        fprintf(stderr, "Out of send buffer\n");
        return 0;
    }

    GLCmdPacketHead *packetHead  = (GLCmdPacketHead *)output_buf;
    packetHead->packet_body_size = 0;
    packetHead->packet_type      = gl_ctrl_type;
    return PACKET_HEAD_LEN;
}

int format_gl_generic_command(
    GLNetworkPacketType gl_packet_type,
    int32_t  packet_size,
    uint8_t *packet_data,
    int32_t  output_buf_size,
    uint8_t *output_buf)
{
    int32_t packet_total_size = PACKET_HEAD_LEN + packet_size;
    if (packet_total_size > output_buf_size) {
        fprintf(stderr, "Out of send buffer\n");
        return 0;
    }

    GLCmdPacketHead *packetHead  = (GLCmdPacketHead *)output_buf;
    packetHead->packet_type      = gl_packet_type;
    packetHead->packet_body_size = packet_size;

    memcpy(output_buf + PACKET_HEAD_LEN, packet_data, packet_size);
    return (PACKET_HEAD_LEN + packet_size);
}

FILE *openDumpFile(const char *name)
{
    return fopen(name, "wb");
}

void writeToDumpFile(FILE *file, uint8_t *data, uint32_t dataLen)
{
    if (file == nullptr)
        return;

    int ret = fwrite(data, dataLen, 1, file);
    assert(ret == 1);
    if (ret != 1) {
        fprintf(stderr, "Cannot write data to file\n");
    }
}

void closeDumpFile(FILE *file)
{
    if (file != nullptr) {
        fclose(file);
    }
}


