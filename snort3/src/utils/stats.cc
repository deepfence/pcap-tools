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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "stats.h"

#include <cassert>

#include "detection/detection_engine.h"
#include "file_api/file_stats.h"
#include "filters/sfthreshold.h"
#include "framework/module.h"
#include "helpers/process.h"
#include "log/messages.h"
#include "main/snort_config.h"
#include "managers/module_manager.h"
#include "packet_io/active.h"
#include "packet_io/sfdaq.h"
#include "packet_io/trough.h"
#include "profiler/profiler.h"
#include "protocols/packet_manager.h"
#include "time/timersub.h"

#include "util.h"

#define STATS_SEPARATOR \
    "--------------------------------------------------"

ProcessCount proc_stats;

namespace snort
{

THREAD_LOCAL PacketCount pc;

//-------------------------------------------------------------------------

static inline void LogSeparator(FILE* fh = stdout)
{
    LogMessage(fh, "%s\n", STATS_SEPARATOR);
}

void LogLabel(const char* s, FILE* fh)
{
    if ( *s == ' ' )
    {
        LogMessage(fh, "%s\n", s);
    }
    else
    {
        LogSeparator(fh);
        LogMessage(fh, "%s\n", s);
    }
}

void LogValue(const char* s, const char* v, FILE* fh)
{
    LogMessage(fh, "%25.25s: %s\n", s, v);
}

void LogCount(const char* s, uint64_t c, FILE* fh)
{
    if ( c )
        LogMessage(fh, "%25.25s: " STDu64 "\n", s, c);
}

void LogStat(const char* s, uint64_t n, uint64_t tot, FILE* fh)
{
    if ( n )
        LogMessage(fh, "%25.25s: " FMTu64("-12") "\t(%7.3f%%)\n", s, n, CalcPct(n, tot));
}

void LogStat(const char* s, double d, FILE* fh)
{
    if ( d )
        LogMessage(fh, "%25.25s: %g\n", s, d);
}
}

using namespace snort;

//-------------------------------------------------------------------------

double CalcPct(uint64_t cnt, uint64_t total)
{
    double pct = 0.0;

    if (total == 0.0)
    {
        pct = (double)cnt;
    }
    else
    {
        pct = (double)cnt / (double)total;
    }

    pct *= 100.0;

    return pct;
}

//-------------------------------------------------------------------------

static struct timeval starttime, endtime;

void TimeStart()
{
    gettimeofday(&starttime, nullptr);
}

void TimeStop()
{
    gettimeofday(&endtime, nullptr);
}

static void timing_stats()
{
    struct timeval difftime;
    TIMERSUB(&endtime, &starttime, &difftime);

    uint32_t tmp = (uint32_t)difftime.tv_sec;
    uint32_t total_secs = tmp;
    if ( total_secs < 1 )
        total_secs = 1;

    uint32_t hrs  = tmp / SECONDS_PER_HOUR;
    tmp  = tmp % SECONDS_PER_HOUR;

    uint32_t mins = tmp / SECONDS_PER_MIN;
    uint32_t secs = tmp % SECONDS_PER_MIN;

    LogLabel("timing");

    LogMessage("%25.25s: %02d:%02d:%02d\n", "runtime", hrs, mins, secs);

    LogMessage("%25.25s: %lu.%06lu\n", "seconds",
        (unsigned long)difftime.tv_sec, (unsigned long)difftime.tv_usec);

    Module* daq = ModuleManager::get_module("daq");
    assert(daq);

    uint64_t num_pkts = (uint64_t)daq->get_global_count("analyzed");
    uint64_t num_byts = (uint64_t)daq->get_global_count("rx_bytes");

    if ( uint64_t pps = (num_pkts / total_secs) )
        LogMessage("%25.25s: " STDu64 "\n", "pkts/sec", pps);

    if ( uint64_t mbps = 8 * num_byts / total_secs / 1024 / 1024 )
        LogMessage("%25.25s: " STDu64 "\n", "Mbits/sec", mbps);
}

//-------------------------------------------------------------------------

const PegInfo pc_names[] =
{
    { CountType::NOW, "analyzed", "total packets processed" },
    { CountType::SUM, "hard_evals", "non-fast pattern rule evaluations" },
    { CountType::SUM, "raw_searches", "fast pattern searches in raw packet data" },
    { CountType::SUM, "cooked_searches", "fast pattern searches in cooked packet data" },
    { CountType::SUM, "pkt_searches", "fast pattern searches in packet data" },
    { CountType::SUM, "alt_searches", "alt fast pattern searches in packet data" },
    { CountType::SUM, "key_searches", "fast pattern searches in key buffer" },
    { CountType::SUM, "header_searches", "fast pattern searches in header buffer" },
    { CountType::SUM, "body_searches", "fast pattern searches in body buffer" },
    { CountType::SUM, "file_searches", "fast pattern searches in file buffer" },
    { CountType::SUM, "raw_key_searches", "fast pattern searches in raw key buffer" },
    { CountType::SUM, "raw_header_searches", "fast pattern searches in raw header buffer" },
    { CountType::SUM, "method_searches", "fast pattern searches in method buffer" },
    { CountType::SUM, "stat_code_searches", "fast pattern searches in status code buffer" },
    { CountType::SUM, "stat_msg_searches", "fast pattern searches in status message buffer" },
    { CountType::SUM, "cookie_searches", "fast pattern searches in cookie buffer" },
    { CountType::SUM, "offloads", "fast pattern searches that were offloaded" },
    { CountType::SUM, "alerts", "alerts not including IP reputation" },
    { CountType::SUM, "total_alerts", "alerts including IP reputation" },
    { CountType::SUM, "logged", "logged packets" },
    { CountType::SUM, "passed", "passed packets" },
    { CountType::SUM, "match_limit", "fast pattern matches not processed" },
    { CountType::SUM, "queue_limit", "events not queued because queue full" },
    { CountType::SUM, "log_limit", "events queued but not logged" },
    { CountType::SUM, "event_limit", "events filtered" },
    { CountType::SUM, "alert_limit", "events previously triggered on same PDU" },
    { CountType::SUM, "context_stalls", "times processing stalled to wait for an available context" },
    { CountType::SUM, "offload_busy", "times offload was not available" },
    { CountType::SUM, "onload_waits", "times processing waited for onload to complete" },
    { CountType::SUM, "offload_fallback", "fast pattern offload search fallback attempts" },
    { CountType::SUM, "offload_failures", "fast pattern offload search failures" },
    { CountType::SUM, "offload_suspends", "fast pattern search suspends due to offload context chains" },
    { CountType::SUM, "pcre_match_limit", "total number of times pcre hit the match limit" },
    { CountType::SUM, "pcre_recursion_limit", "total number of times pcre hit the recursion limit" },
    { CountType::SUM, "pcre_error", "total number of times pcre returns error" },
    { CountType::END, nullptr, nullptr }
};

const PegInfo proc_names[] =
{
    { CountType::SUM, "local_commands", "total local commands processed" },
    { CountType::SUM, "remote_commands", "total remote commands processed" },
    { CountType::SUM, "signals", "total signals processed" },
    { CountType::SUM, "conf_reloads", "number of times configuration was reloaded" },
    { CountType::SUM, "policy_reloads", "number of times policies were reloaded" },
    { CountType::SUM, "inspector_deletions", "number of times inspectors were deleted" },
    { CountType::SUM, "daq_reloads", "number of times daq configuration was reloaded" },
    { CountType::SUM, "attribute_table_reloads", "number of times hosts attribute table was reloaded" },
    { CountType::SUM, "attribute_table_hosts", "number of hosts added to the attribute table" },
    { CountType::SUM, "attribute_table_overflow", "number of host additions that failed due to attribute table full" },
    { CountType::END, nullptr, nullptr }
};

//-------------------------------------------------------------------------

void DropStats()
{
    LogLabel("Packet Statistics");
    ModuleManager::get_module("daq")->show_stats();

    PacketManager::dump_stats();

    LogLabel("Module Statistics");
    const char* exclude = "daq snort";
    ModuleManager::dump_stats(exclude, false);
    ModuleManager::dump_stats(exclude, true);

    LogLabel("Summary Statistics");
    show_stats((PegCount*)&proc_stats, proc_names, array_size(proc_names)-1, "process");

}

//-------------------------------------------------------------------------

void PrintStatistics()
{
    DropStats();
    timing_stats();

    SnortConfig* sc = SnortConfig::get_main_conf();

    // FIXIT-L can do flag saving with RAII (much cleaner)
    int save_quiet_flag = sc->logging_flags & LOGGING_FLAG__QUIET;
    sc->logging_flags &= ~LOGGING_FLAG__QUIET;

    // once more for the main thread
    Profiler::consolidate_stats();
    Profiler::show_stats();

    sc->logging_flags |= save_quiet_flag;
}

//-------------------------------------------------------------------------

void sum_stats(
    PegCount* gpegs, PegCount* tpegs, unsigned n)
{
    for ( unsigned i = 0; i < n; ++i )
    {
        gpegs[i] += tpegs[i];
        tpegs[i] = 0;
    }
}

static bool show_stat(
    bool head, PegCount count, const char* name, const char* module_name,
    FILE* fh = stdout)
{
    if ( !count )
        return head;

    if ( module_name && !head )
    {
        LogLabel(module_name, fh);
        head = true;
    }

    LogCount(name, count, fh);
    return head;
}

void show_stats(PegCount* pegs, const PegInfo* info, const char* module_name)
{
    bool head = false;

    for ( unsigned i = 0; info->name[i]; ++i )
        head = show_stat(head, pegs[i], info[i].name, module_name);
}

void show_stats(
    PegCount* pegs, const PegInfo* info, unsigned n, const char* module_name)
{
    bool head = false;

    for ( unsigned i = 0; i < n; ++i )
        head = show_stat(head, pegs[i], info[i].name, module_name);
}

void show_stats(
    PegCount* pegs, const PegInfo* info,
    const IndexVec& peg_idxs, const char* module_name, FILE* fh)
{
    bool head = false;

    for ( auto& i : peg_idxs)
        head = show_stat(head, pegs[i], info[i].name, module_name, fh);
}

void show_percent_stats(
    PegCount* pegs, const char* names[], unsigned n, const char* module_name)
{
    bool head = false;

    for ( unsigned i = 0; i < n; ++i )
    {
        PegCount c = pegs[i];
        const char* s = names[i];

        if ( !c )
            continue;

        if ( module_name && !head )
        {
            LogLabel(module_name);
            head = true;
        }

        LogStat(s, c, pegs[0], stdout);
    }
}
