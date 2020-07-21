//--------------------------------------------------------------------------
// Copyright (C) 2019-2020 Cisco and/or its affiliates. All rights reserved.
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License Version 2 as published
// by the Free Software Foundation.  You may not use, modify or distribute
// this program under any other version of the GNU General Public License.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//--------------------------------------------------------------------------
// http2_settings_frame.cc author Deepak Ramadass <deramada@cisco.com>

#ifndef HTTP2_SETTINGS_FRAME_H
#define HTTP2_SETTINGS_FRAME_H

#include "http2_frame.h"

class Field;
class Http2ConnectionSettings;
class Http2Frame;

class Http2SettingsFrame : public Http2Frame
{
public:
    friend Http2Frame* Http2Frame::new_frame(const uint8_t*, const int32_t, const uint8_t*,
        const int32_t, Http2FlowData*, HttpCommon::SourceId, Http2Stream* stream);
    bool is_detection_required() const override { return false; }

#ifdef REG_TEST
    void print_frame(FILE* output) override;
#endif

private:
    Http2SettingsFrame(const uint8_t* header_buffer, const int32_t header_len,
        const uint8_t* data_buffer, const int32_t data_len, Http2FlowData* ssn_data,
        HttpCommon::SourceId src_id, Http2Stream* stream);

    void parse_settings_frame();
    bool sanity_check();
    void handle_update(uint16_t id, uint32_t value);

    bool bad_frame = false;
    static const uint8_t SfAck = 0x01;
};

class Http2ConnectionSettings
{
public:
    uint32_t get_param(uint16_t id);
    void set_param(uint16_t id, uint32_t value);

private:
    void validate_param_id(uint16_t id);

    static const uint16_t PARAMETER_COUNT = 6;
    uint32_t parameters[PARAMETER_COUNT] = {
             4096, // Header table size
                1, // Push promise
              100, // Max concurrent Streams
            65535, // Window size
            16384, // Max frame size
       4294967295  // Max header list size
    };
};
#endif
