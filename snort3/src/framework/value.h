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
// value.h author Russ Combs <rucombs@cisco.com>

#ifndef VALUE_H
#define VALUE_H

// Value is used to represent Lua bool, number, and string.

#include <algorithm>
#include <cstring>
#include <sstream>

#include "framework/bits.h"
#include "framework/parameter.h"
#include "main/snort_types.h"

namespace snort
{
struct SfCidr;
struct SfIp;

class SO_PUBLIC Value
{
public:
    static const unsigned mask_bits = 52; // ieee 754 significand
    enum ValueType { VT_BOOL, VT_NUM, VT_STR };

    Value(bool b)
    { set(b); init(); }

    Value(double d)
    { set(d); init(); }

    Value(const char* s)
    { set(s); init(); }

    ValueType get_type()
    { return type; }

    ~Value()
    {
        if ( ss )
            delete ss;
    }

    void set(bool b)
    { type = VT_BOOL; num = b ? 1 : 0; str.clear(); }

    void set(double d)
    { type = VT_NUM; num = d; str.clear(); }

    void set(long n)
    { set((double)n); }

    void set(const char* s)
    { type = VT_STR; str = s; num = 0; }

    void set(const uint8_t* s, unsigned len)
    { type = VT_STR; str.assign((const char*)s, len); num = 0; }

    void set(const Parameter* p)
    { param = p; }

    void set_enum(unsigned u)
    { type = VT_NUM; num = u;  }

    void set_aux(uint64_t u)
    { num = (double)u; }

    const char* get_name() const
    { return param ? param->name : nullptr; }

    bool is(const char* s) const
    { return param ? !strcmp(param->name, s) : false; }

    bool get_bool() const
    { return num != 0; }

    long get_long() const  // FIXIT-L to be removed
    { return (long)num; }

    size_t get_size() const
    { return (size_t)num; }

    uint8_t get_uint8() const
    { return (uint8_t)num; }

    int16_t get_int16() const
    { return (int16_t)num; }

    uint16_t get_uint16() const
    { return (uint16_t)num; }

    int32_t get_int32() const
    { return (int32_t)num; }

    uint32_t get_uint32() const
    { return (uint32_t)num; }

    int64_t get_int64() const
    { return (int64_t)num; }

    uint64_t get_uint64() const
    { return (uint64_t)num; }

    double get_real() const
    { return num; }

    const uint8_t* get_buffer(unsigned& n) const
    { n = (unsigned)str.size(); return (const uint8_t*)str.data(); }

    const char* get_string() const
    { return str.c_str(); }

    const char* get_as_string();

    bool strtol(long&) const;
    bool strtol(long&, const std::string&) const;

    bool operator==(const char* s) const
    { return str == s; }

    bool operator==(long n) const
    { return (long)num == n; }

    bool operator==(double d) const
    { return num == d; }

    void get_bits(PortBitSet&) const;
    void get_bits(VlanBitSet&) const;
    void get_bits(ByteBitSet&) const;
    void get_bits(ZoneBitSet&) const;

    void lower()
    { std::transform(str.begin(), str.end(), str.begin(), ::tolower); }

    void upper()
    { std::transform(str.begin(), str.end(), str.begin(), ::toupper); }

    uint32_t get_ip4() const;
    void get_mac(uint8_t (&mac)[6]) const;
    void get_addr(uint8_t (&addr)[16]) const;
    void get_addr_ip4(uint8_t (&addr)[4]) const;
    void get_addr_ip6(uint8_t (&addr)[16]) const;
    void get_addr(SfIp&) const;
    void get_addr(SfCidr&) const;

    void set_first_token();
    bool get_next_token(std::string&);
    bool get_next_csv_token(std::string&);

    // set/clear flag based on get_bool()
    void update_mask(uint8_t& mask, uint8_t flag, bool invert = false);
    void update_mask(uint16_t& mask, uint16_t flag, bool invert = false);
    void update_mask(uint32_t& mask, uint32_t flag, bool invert = false);
    void update_mask(uint64_t& mask, uint64_t flag, bool invert = false);

private:
    void init()
    { param = nullptr; ss = nullptr; }

private:
    ValueType type;
    double num;
    std::string str;
    std::stringstream* ss;
    const Parameter* param;
};
}
#endif

