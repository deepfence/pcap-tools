//--------------------------------------------------------------------------
// Copyright (C) 2018-2020 Cisco and/or its affiliates. All rights reserved.
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
// http2_module.cc author Tom Peters <thopeter@cisco.com>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "http2_module.h"

using namespace snort;
using namespace Http2Enums;

const Parameter Http2Module::http2_params[] =
{
#ifdef REG_TEST
    { "test_input", Parameter::PT_BOOL, nullptr, "false",
      "read HTTP/2 messages from text file" },

    { "test_output", Parameter::PT_BOOL, nullptr, "false",
      "print out HTTP section data" },

    { "print_amount", Parameter::PT_INT, "1:max53", "1200",
      "number of characters to print from a Field" },

    { "print_hex", Parameter::PT_BOOL, nullptr, "false",
      "nonprinting characters printed in [HH] format instead of using an asterisk" },

    { "show_pegs", Parameter::PT_BOOL, nullptr, "true",
      "display peg counts with test output" },

    { "show_scan", Parameter::PT_BOOL, nullptr, "false",
      "display scanned segments" },
#endif

    { nullptr, Parameter::PT_MAX, nullptr, nullptr, nullptr }
};

THREAD_LOCAL ProfileStats Http2Module::http2_profile;

ProfileStats* Http2Module::get_profile() const
{ return &http2_profile; }

THREAD_LOCAL PegCount Http2Module::peg_counts[PEG_COUNT__MAX] = { 0 };

bool Http2Module::begin(const char*, int, SnortConfig*)
{
    delete params;
    params = new Http2ParaList;
    return true;
}

bool Http2Module::set(const char*, Value& val, SnortConfig*)
{
#ifdef REG_TEST
    if (val.is("test_input"))
    {
        params->test_input = val.get_bool();
    }
    else if (val.is("test_output"))
    {
        params->test_output = val.get_bool();
    }
    else if (val.is("print_amount"))
    {
        params->print_amount = val.get_int64();
    }
    else if (val.is("print_hex"))
    {
        params->print_hex = val.get_bool();
    }
    else if (val.is("show_pegs"))
    {
        params->show_pegs = val.get_bool();
    }
    else if (val.is("show_scan"))
    {
        params->show_scan = val.get_bool();
    }
    else
    {
        return false;
    }
    return true;
#else
    UNUSED(val);
    return false;
#endif
}

bool Http2Module::end(const char*, int, SnortConfig*)
{
    return true;
}

