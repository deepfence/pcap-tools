//--------------------------------------------------------------------------
// Copyright (C) 2014-2020 Cisco and/or its affiliates. All rights reserved.
// Copyright (C) 2013-2013 Sourcefire, Inc.
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

#ifndef STATS_H
#define STATS_H

// Provides facilities for displaying Snort exit stats

#include <daq_common.h>
#include <vector>

#include "framework/counts.h"
#include "main/snort_types.h"
#include "main/thread.h"

using IndexVec = std::vector<unsigned>;

// FIXIT-L split this out into appropriate modules
struct PacketCount
{
    PegCount analyzed_pkts;
    PegCount hard_evals;
    PegCount raw_searches;
    PegCount cooked_searches;
    PegCount pkt_searches;
    PegCount alt_searches;
    PegCount key_searches;
    PegCount header_searches;
    PegCount body_searches;
    PegCount file_searches;
    PegCount raw_key_searches;
    PegCount raw_header_searches;
    PegCount method_searches;
    PegCount stat_code_searches;
    PegCount stat_msg_searches;
    PegCount cookie_searches;
    PegCount offloads;
    PegCount alert_pkts;
    PegCount total_alert_pkts;
    PegCount log_pkts;
    PegCount pass_pkts;
    PegCount match_limit;
    PegCount queue_limit;
    PegCount log_limit;
    PegCount event_limit;
    PegCount alert_limit;
    PegCount context_stalls;
    PegCount offload_busy;
    PegCount onload_waits;
    PegCount offload_fallback;
    PegCount offload_failures;
    PegCount offload_suspends;
    PegCount pcre_match_limit;
    PegCount pcre_recursion_limit;
    PegCount pcre_error;
};

struct ProcessCount
{
    PegCount local_commands;
    PegCount remote_commands;
    PegCount signals;
    PegCount conf_reloads;
    PegCount policy_reloads;
    PegCount inspector_deletions;
    PegCount daq_reloads;
    PegCount attribute_table_reloads;
    PegCount attribute_table_hosts;
    PegCount attribute_table_overflow;
};

extern ProcessCount proc_stats;

extern const PegInfo daq_names[];
extern const PegInfo pc_names[];
extern const PegInfo proc_names[];

namespace snort
{
extern SO_PUBLIC THREAD_LOCAL PacketCount pc;

SO_PUBLIC inline PegCount get_packet_number() { return pc.analyzed_pkts; }

SO_PUBLIC void LogLabel(const char*, FILE* = stdout);
SO_PUBLIC void LogValue(const char*, const char*, FILE* = stdout);
SO_PUBLIC void LogCount(const char*, uint64_t, FILE* = stdout);

SO_PUBLIC void LogStat(const char*, uint64_t n, uint64_t tot, FILE* = stdout);
SO_PUBLIC void LogStat(const char*, double, FILE* = stdout);
}

void sum_stats(PegCount* sums, PegCount* counts, unsigned n);
void show_stats(PegCount*, const PegInfo*, const char* module_name = nullptr);
void show_stats(PegCount*, const PegInfo*, unsigned n, const char* module_name = nullptr);
void show_stats(PegCount*, const PegInfo*, const IndexVec&, const char* module_name, FILE*);
void show_percent_stats(PegCount*, const char*[], unsigned n, const char* module_name = nullptr);

void sum_stats(SimpleStats* sums, SimpleStats* counts);
void show_stats(SimpleStats*, const char* module_name);

double CalcPct(uint64_t, uint64_t);
void DropStats();
void PrintStatistics();
void TimeStart();
void TimeStop();

#endif
