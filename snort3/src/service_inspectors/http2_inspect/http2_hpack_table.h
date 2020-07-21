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
// http2_hpack_table.h author Katura Harvey <katharve@cisco.com>

#ifndef HTTP2_HPACK_TABLE_H
#define HTTP2_HPACK_TABLE_H

#include "main/snort_types.h"

#include "http2_enum.h"
#include "http2_hpack_dynamic_table.h"

class Http2FlowData;

struct HpackTableEntry
{
    HpackTableEntry(uint32_t name_len, const uint8_t* _name, uint32_t value_len,
        const uint8_t* _value) : name { static_cast<int32_t>(name_len), _name },
        value { static_cast<int32_t>(value_len), _value } { }
    HpackTableEntry(const Field& copy_name, const Field& copy_value);
    Field name;
    Field value;
};

class HpackIndexTable
{
public:
    HpackIndexTable(Http2FlowData* flow_data, HttpCommon::SourceId src_id) :
        session_data(flow_data), source_id(src_id) { }
    const HpackTableEntry* lookup(uint64_t index) const;
    bool add_index(const Field& name, const Field& value);
    bool hpack_table_size_update(const uint32_t size);
    void settings_table_size_update(const uint32_t size);

    const static uint8_t STATIC_MAX_INDEX = 61;
    const static uint8_t PSEUDO_HEADER_MAX_STATIC_INDEX = 14;

private:
    const static HpackTableEntry static_table[STATIC_MAX_INDEX + 1];
    HpackDynamicTable dynamic_table;
    Http2FlowData* session_data;
    HttpCommon::SourceId source_id;
    bool encoder_set_max_size = false;
};
#endif
