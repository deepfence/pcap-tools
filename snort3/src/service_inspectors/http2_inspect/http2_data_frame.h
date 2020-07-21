//--------------------------------------------------------------------------
// Copyright (C) 2020-2020 Cisco and/or its affiliates. All rights reserved.
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
// http2_data_frame.h author Maya Dagon <mdagon@cisco.com>

#ifndef HTTP2_DATA_FRAME_H
#define HTTP2_DATA_FRAME_H

#include "http2_frame.h"

class Http2Frame;
class Http2Stream;

class Http2DataFrame : public Http2Frame
{
public:
    ~Http2DataFrame() override {}
    void clear() override;

    uint32_t get_xtradata_mask() override { return xtradata_mask; }
    bool is_detection_required() const override { return detection_required; }
    void update_stream_state() override;

    friend Http2Frame* Http2Frame::new_frame(const uint8_t*, const int32_t, const uint8_t*,
        const int32_t, Http2FlowData*, HttpCommon::SourceId, Http2Stream* stream);

#ifdef REG_TEST
    void print_frame(FILE* output) override;
#endif

private:
    Http2DataFrame(const uint8_t* header_buffer, const int32_t header_len,
        const uint8_t* data_buffer, const int32_t data_len, Http2FlowData* ssn_data,
        HttpCommon::SourceId src_id, Http2Stream* stream);
    uint32_t data_length = 0;
    uint32_t xtradata_mask = 0;
    bool detection_required = false;
};
#endif
