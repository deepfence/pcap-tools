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
// http_cutter.h author Tom Peters <thopeter@cisco.com>

#ifndef HTTP_CUTTER_H
#define HTTP_CUTTER_H

#include <cassert>
#include <zlib.h>

#include "http_enum.h"
#include "http_event.h"

//-------------------------------------------------------------------------
// HttpCutter class and subclasses
//-------------------------------------------------------------------------

class HttpCutter
{
public:
    virtual ~HttpCutter() = default;
    virtual HttpEnums::ScanResult cut(const uint8_t* buffer, uint32_t length,
        HttpInfractions* infractions, HttpEventGen* events, uint32_t flow_target, bool stretch,
        bool h2_body_finished) = 0;
    uint32_t get_num_flush() const { return num_flush; }
    uint32_t get_octets_seen() const { return octets_seen; }
    uint32_t get_num_excess() const { return num_crlf; }
    virtual uint32_t get_num_head_lines() const { return 0; }
    virtual bool get_is_broken_chunk() const { return false; }
    virtual uint32_t get_num_good_chunks() const { return 0; }
    virtual void soft_reset() {}

protected:
    // number of octets processed by previous cut() calls that returned NOT_FOUND
    uint32_t octets_seen = 0;
    uint32_t num_crlf = 0;
    uint32_t num_flush = 0;
};

class HttpStartCutter : public HttpCutter
{
public:
    HttpEnums::ScanResult cut(const uint8_t* buffer, uint32_t length,
        HttpInfractions* infractions, HttpEventGen* events, uint32_t, bool, bool) override;

protected:
    enum ValidationResult { V_GOOD, V_BAD, V_TBD };

private:
    static const int MAX_LEADING_WHITESPACE = 20;
    virtual ValidationResult validate(uint8_t octet, HttpInfractions*, HttpEventGen*) = 0;
    bool validated = false;
};

class HttpRequestCutter : public HttpStartCutter
{
private:
    uint32_t octets_checked = 0;
    ValidationResult validate(uint8_t octet, HttpInfractions*, HttpEventGen*) override;
};

class HttpStatusCutter : public HttpStartCutter
{
private:
    uint32_t octets_checked = 0;
    ValidationResult validate(uint8_t octet, HttpInfractions*, HttpEventGen*) override;
};

class HttpHeaderCutter : public HttpCutter
{
public:
    HttpEnums::ScanResult cut(const uint8_t* buffer, uint32_t length,
        HttpInfractions* infractions, HttpEventGen* events, uint32_t, bool, bool) override;
    uint32_t get_num_head_lines() const override { return num_head_lines; }

private:
    enum LineEndState { ZERO, HALF, ONE, THREEHALF };
    LineEndState state = ONE;
    int32_t num_head_lines = 0;
};

class HttpBodyCutter : public HttpCutter
{
public:
    HttpBodyCutter(bool detained_inspection_, HttpEnums::CompressId compression_);
    ~HttpBodyCutter() override;
    void soft_reset() override { octets_seen = 0; packet_detained = false; }
    void detain_ended() { packet_detained = false; }

protected:
    bool need_detained_inspection(const uint8_t* data, uint32_t length);

private:
    bool dangerous(const uint8_t* data, uint32_t length);

    const bool detained_inspection;
    bool packet_detained = false;
    uint8_t partial_match = 0;
    bool detention_required = false;
    HttpEnums::CompressId compression;
    z_stream* compress_stream = nullptr;
};

class HttpBodyClCutter : public HttpBodyCutter
{
public:
    HttpBodyClCutter(int64_t expected_length, bool detained_inspection,
        HttpEnums::CompressId compression) :
        HttpBodyCutter(detained_inspection, compression), remaining(expected_length)
        { assert(remaining > 0); }
    HttpEnums::ScanResult cut(const uint8_t*, uint32_t length, HttpInfractions*, HttpEventGen*,
        uint32_t flow_target, bool stretch, bool) override;

private:
    int64_t remaining;
};

class HttpBodyOldCutter : public HttpBodyCutter
{
public:
    explicit HttpBodyOldCutter(bool detained_inspection, HttpEnums::CompressId compression) :
        HttpBodyCutter(detained_inspection, compression) {}
    HttpEnums::ScanResult cut(const uint8_t*, uint32_t, HttpInfractions*, HttpEventGen*,
        uint32_t flow_target, bool stretch, bool) override;
};

class HttpBodyChunkCutter : public HttpBodyCutter
{
public:
    explicit HttpBodyChunkCutter(bool detained_inspection, HttpEnums::CompressId compression) :
        HttpBodyCutter(detained_inspection, compression) {}
    HttpEnums::ScanResult cut(const uint8_t* buffer, uint32_t length,
        HttpInfractions* infractions, HttpEventGen* events, uint32_t flow_target, bool stretch,
        bool) override;
    bool get_is_broken_chunk() const override { return curr_state == HttpEnums::CHUNK_BAD; }
    uint32_t get_num_good_chunks() const override { return num_good_chunks; }
    void soft_reset() override { num_good_chunks = 0; HttpBodyCutter::soft_reset(); }

private:
    uint32_t data_seen = 0;
    HttpEnums::ChunkState curr_state = HttpEnums::CHUNK_NEWLINES;
    uint32_t expected = 0;
    uint32_t num_leading_ws = 0;
    uint32_t num_zeros = 0;
    uint32_t digits_seen = 0;
    uint32_t num_good_chunks = 0;  // that end in the current section
};

class HttpBodyH2Cutter : public HttpBodyCutter
{
public:
    explicit HttpBodyH2Cutter(int64_t expected_length, bool detained_inspection,
        HttpEnums::CompressId compression) :
        HttpBodyCutter(detained_inspection, compression), expected_body_length(expected_length) {}
    HttpEnums::ScanResult cut(const uint8_t*, uint32_t, HttpInfractions*, HttpEventGen*,
        uint32_t flow_target, bool stretch, bool h2_body_finished) override;
private:
    int64_t expected_body_length;
    uint32_t total_octets_scanned = 0;
};


#endif

