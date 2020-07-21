//--------------------------------------------------------------------------
// Copyright (C) 2016-2020 Cisco and/or its affiliates. All rights reserved.
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

// memory_module.cc author Joel Cornett <jocornet@cisco.com>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "memory_module.h"

#include "main/snort_config.h"

#include "memory_config.h"

using namespace snort;

// -----------------------------------------------------------------------------
// memory attributes
// -----------------------------------------------------------------------------

#define s_name "memory"
#define s_help \
    "memory management configuration"

static const Parameter s_params[] =
{
    { "cap", Parameter::PT_INT, "0:maxSZ", "0",
        "set the per-packet-thread cap on memory (bytes, 0 to disable)" },

    { "threshold", Parameter::PT_INT, "0:100", "0",
        "set the per-packet-thread threshold for preemptive cleanup actions "
        "(percent, 0 to disable)" },

    { nullptr, Parameter::PT_MAX, nullptr, nullptr, nullptr }
};

THREAD_LOCAL MemoryCounts mem_stats;
static MemoryCounts zero_stats = { };

const PegInfo mem_pegs[] =
{
    { CountType::NOW, "allocations", "total number of allocations" },
    { CountType::NOW, "deallocations", "total number of deallocations" },
    { CountType::NOW, "allocated", "total amount of memory allocated" },
    { CountType::NOW, "deallocated", "total amount of memory allocated" },
    { CountType::NOW, "reap_attempts", "attempts to reclaim memory" },
    { CountType::NOW, "reap_failures", "failures to reclaim memory" },
    { CountType::MAX, "max_in_use", "highest allocated - deallocated" },
    { CountType::NOW, "total_fudge", "sum of all adjustments" },
    { CountType::END, nullptr, nullptr }
};

// -----------------------------------------------------------------------------
// memory module
// -----------------------------------------------------------------------------

bool MemoryModule::configured = false;

MemoryModule::MemoryModule() :
    Module(s_name, s_help, s_params)
{ }

bool MemoryModule::set(const char*, Value& v, SnortConfig* sc)
{
    if ( v.is("cap") )
        sc->memory->cap = v.get_size();

    else if ( v.is("threshold") )
        sc->memory->threshold = v.get_uint8();

    else
        return false;

    return true;
}

bool MemoryModule::end(const char*, int, SnortConfig*)
{
    configured = true;
    return true;
}

bool MemoryModule::is_active()
{ return configured; }

const PegInfo* MemoryModule::get_pegs() const
{ return mem_pegs; }

PegCount* MemoryModule::get_counts() const
{ return is_active() ? (PegCount*)&mem_stats : (PegCount*)&zero_stats; }

