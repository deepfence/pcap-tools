//--------------------------------------------------------------------------
// Copyright (C) 2014-2020 Cisco and/or its affiliates. All rights reserved.
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
// http_flow_data.h author Tom Peters <thopeter@cisco.com>

#ifndef HTTP_FLOW_DATA_H
#define HTTP_FLOW_DATA_H

#include <zlib.h>

#include <cstdio>

#include "flow/flow.h"
#include "mime/file_mime_process.h"
#include "utils/util_utf.h"
#include "decompress/file_decomp.h"

#include "http_common.h"
#include "http_enum.h"
#include "http_event.h"

class HttpTransaction;
class HttpJsNorm;
class HttpMsgSection;
class HttpCutter;
class HttpQueryParser;

class HttpFlowData : public snort::FlowData
{
public:
    HttpFlowData();
    ~HttpFlowData() override;
    static unsigned inspector_id;
    static void init() { inspector_id = snort::FlowData::create_flow_data_id(); }
    size_t size_of() override { return sizeof(*this); }

    friend class HttpInspect;
    friend class HttpMsgSection;
    friend class HttpMsgStart;
    friend class HttpMsgRequest;
    friend class HttpMsgStatus;
    friend class HttpMsgHeader;
    friend class HttpMsgHeadShared;
    friend class HttpMsgTrailer;
    friend class HttpMsgBody;
    friend class HttpMsgBodyChunk;
    friend class HttpMsgBodyCl;
    friend class HttpMsgBodyH2;
    friend class HttpMsgBodyOld;
    friend class HttpQueryParser;
    friend class HttpStreamSplitter;
    friend class HttpTransaction;
#if defined(REG_TEST) || defined(UNIT_TEST)
    friend class HttpUnitTestSetup;
#endif

    HttpEnums::SectionType get_type_expected(HttpCommon::SourceId source_id)
    { return type_expected[source_id]; }

    void finish_h2_body(HttpCommon::SourceId source_id)
    { h2_body_finished[source_id] = true; }

    void reset_partial_flush(HttpCommon::SourceId source_id)
    { partial_flush[source_id] = false; }

    static uint16_t get_memory_usage_estimate();

private:
    // HTTP/2 handling
    bool for_http2 = false;
    bool h2_body_finished[2] = { false, false };

    // Convenience routines
    void half_reset(HttpCommon::SourceId source_id);
    void trailer_prep(HttpCommon::SourceId source_id);
    void garbage_collect();

    // 0 element refers to client request, 1 element refers to server response

    // *** StreamSplitter internal data - scan()
    HttpCutter* cutter[2] = { nullptr, nullptr };

    // *** StreamSplitter internal data - reassemble()
    uint8_t* section_buffer[2] = { nullptr, nullptr };
    uint32_t section_offset[2] = { 0, 0 };
    uint32_t chunk_expected_length[2] = { 0, 0 };
    uint32_t running_total[2] = { 0, 0 };
    HttpEnums::ChunkState chunk_state[2] = { HttpEnums::CHUNK_NEWLINES,
        HttpEnums::CHUNK_NEWLINES };
    uint32_t partial_raw_bytes[2] = { 0, 0 };
    uint8_t* partial_buffer[2] = { nullptr, nullptr };
    uint32_t partial_buffer_length[2] = { 0, 0 };

    // *** StreamSplitter internal data - scan() => reassemble()
    uint32_t num_excess[2] = { 0, 0 };
    uint32_t num_good_chunks[2] = { 0, 0 };
    uint32_t octets_expected[2] = { 0, 0 };
    bool is_broken_chunk[2] = { false, false };

    // *** StreamSplitter => Inspector (facts about the most recent message section)
    HttpEnums::SectionType section_type[2] = { HttpEnums::SEC__NOT_COMPUTE,
                                                HttpEnums::SEC__NOT_COMPUTE };
    int32_t octets_reassembled[2] = { HttpCommon::STAT_NOT_PRESENT, HttpCommon::STAT_NOT_PRESENT };
    int32_t num_head_lines[2] = { HttpCommon::STAT_NOT_PRESENT, HttpCommon::STAT_NOT_PRESENT };
    bool tcp_close[2] = { false, false };
    bool partial_flush[2] = { false, false };
    uint64_t last_connect_trans_w_early_traffic = 0;

    HttpInfractions* infractions[2] = { new HttpInfractions, new HttpInfractions };
    HttpEventGen* events[2] = { new HttpEventGen, new HttpEventGen };

    // Infractions are associated with a specific message and are stored in the transaction for
    // that message. But StreamSplitter splits the start line before there is a transaction and
    // needs a place to put the problems it finds. Hence infractions are created before there is a
    // transaction to associate them with and stored here until attach_my_transaction() takes them
    // away and resets these to nullptr. The accessor method hides this from StreamSplitter.
    HttpInfractions* get_infractions(HttpCommon::SourceId source_id);

    // *** Inspector => StreamSplitter (facts about the message section that is coming next)
    HttpEnums::SectionType type_expected[2] = { HttpEnums::SEC_REQUEST, HttpEnums::SEC_STATUS };
    uint64_t last_request_was_connect = false;
    // length of the data from Content-Length field
    z_stream* compress_stream[2] = { nullptr, nullptr };
    uint64_t zero_nine_expected = 0;
    int64_t data_length[2] = { HttpCommon::STAT_NOT_PRESENT, HttpCommon::STAT_NOT_PRESENT };
    uint32_t section_size_target[2] = { 0, 0 };
    HttpEnums::CompressId compression[2] = { HttpEnums::CMP_NONE, HttpEnums::CMP_NONE };
    HttpEnums::DetectionStatus detection_status[2] = { HttpEnums::DET_ON, HttpEnums::DET_ON };
    bool stretch_section_to_packet[2] = { false, false };
    bool detained_inspection[2] = { false, false };

    // *** Inspector's internal data about the current message
    struct FdCallbackContext
    {
        HttpInfractions* infractions = nullptr;
        HttpEventGen* events = nullptr;
    };
    FdCallbackContext fd_alert_context; // SRC_SERVER only
    snort::MimeSession* mime_state[2] = { nullptr, nullptr };
    snort::UtfDecodeSession* utf_state = nullptr; // SRC_SERVER only
    fd_session_t* fd_state = nullptr; // SRC_SERVER only
    int64_t file_depth_remaining[2] = { HttpCommon::STAT_NOT_PRESENT,
        HttpCommon::STAT_NOT_PRESENT };
    int64_t detect_depth_remaining[2] = { HttpCommon::STAT_NOT_PRESENT,
        HttpCommon::STAT_NOT_PRESENT };
    uint64_t expected_trans_num[2] = { 1, 1 };

    // number of user data octets seen so far (regular body or chunks)
    int64_t body_octets[2] = { HttpCommon::STAT_NOT_PRESENT, HttpCommon::STAT_NOT_PRESENT };
    int32_t status_code_num = HttpCommon::STAT_NOT_PRESENT;
    HttpEnums::VersionId version_id[2] = { HttpEnums::VERS__NOT_PRESENT,
                                            HttpEnums::VERS__NOT_PRESENT };
    HttpEnums::MethodId method_id = HttpEnums::METH__NOT_PRESENT;

    bool cutover_on_clear = false;

    // *** Transaction management including pipelining
    static const int MAX_PIPELINE = 100;  // requests seen - responses seen <= MAX_PIPELINE
    HttpTransaction* transaction[2] = { nullptr, nullptr };
    HttpTransaction** pipeline = nullptr;
    int16_t pipeline_front = 0;
    int16_t pipeline_back = 0;
    bool pipeline_overflow = false;
    bool pipeline_underflow = false;

    bool add_to_pipeline(HttpTransaction* latest);
    HttpTransaction* take_from_pipeline();
    void delete_pipeline();

    // Transactions with uncleared sections awaiting deletion
    HttpTransaction* discard_list = nullptr;

    // Estimates of how much memory http_inspect uses to process a stream for H2I
    static const uint16_t header_size_estimate = 3000;  // combined raw size of request and
                                                        //response message headers
    static const uint16_t small_things = 400; // minor memory costs not otherwise accounted for
    static const uint16_t memory_usage_estimate;

#ifdef REG_TEST
    static uint64_t instance_count;
    uint64_t seq_num;

    void show(FILE* out_file) const;
#endif
};

#endif

