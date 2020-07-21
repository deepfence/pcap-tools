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

// stream_module.h author Russ Combs <rucombs@cisco.com>

#ifndef STREAM_MODULE_H
#define STREAM_MODULE_H

#include "main/analyzer.h"
#include "main/snort_config.h"
#include "flow/flow_config.h"
#include "flow/flow_control.h"
#include "framework/module.h"

namespace snort
{
class Trace;
struct SnortConfig;
}

extern THREAD_LOCAL snort::ProfileStats s5PerfStats;
extern THREAD_LOCAL class FlowControl* flow_con;
extern THREAD_LOCAL const snort::Trace* stream_trace;

//-------------------------------------------------------------------------
// stream module
//-------------------------------------------------------------------------

#define MOD_NAME "stream"
#define MOD_HELP "common flow tracking"

struct BaseStats
{
     PegCount flows;
     PegCount prunes;
     PegCount timeout_prunes;
     PegCount excess_prunes;
     PegCount uni_prunes;
     PegCount preemptive_prunes;
     PegCount memcap_prunes;
     PegCount ha_prunes;
     PegCount stale_prunes;
     PegCount expected_flows;
     PegCount expected_realized;
     PegCount expected_pruned;
     PegCount expected_overflows;
     PegCount reload_tuning_idle;
     PegCount reload_tuning_packets;
     PegCount reload_total_adds;
     PegCount reload_total_deletes;
     PegCount reload_freelist_flow_deletes;
     PegCount reload_allowed_flow_deletes;
     PegCount reload_blocked_flow_deletes;
     PegCount reload_offloaded_flow_deletes;
};

extern const PegInfo base_pegs[];

extern THREAD_LOCAL BaseStats stream_base_stats;

struct StreamModuleConfig
{
    FlowCacheConfig flow_cache_cfg;
#ifdef REG_TEST
    unsigned footprint = 0;
#endif
    uint32_t held_packet_timeout = 1000;  // in milliseconds

    void show() const;
};

class StreamReloadResourceManager : public snort::ReloadResourceTuner
{
public:
    StreamReloadResourceManager() {}

    bool tinit() override;
    bool tune_packet_context() override;
    bool tune_idle_context() override;

    bool initialize(const StreamModuleConfig&);

private:
    bool tune_resources(unsigned work_limit);

private:
    StreamModuleConfig config;
};

class HPQReloadTuner : public snort::ReloadResourceTuner
{
public:
    HPQReloadTuner() = default;

    bool tinit() override;
    bool tune_packet_context() override;
    bool tune_idle_context() override;
    bool initialize(uint32_t new_timeout_ms);

private:
    uint32_t held_packet_timeout;
    timeval reload_time;
};

class StreamModule : public snort::Module
{
public:
    StreamModule();

    bool begin(const char*, int, snort::SnortConfig*) override;
    bool set(const char*, snort::Value&, snort::SnortConfig*) override;
    bool end(const char*, int, snort::SnortConfig*) override;

    const PegInfo* get_pegs() const override;
    PegCount* get_counts() const override;
    snort::ProfileStats* get_profile() const override;
    const StreamModuleConfig* get_data();

    unsigned get_gid() const override;
    const snort::RuleMap* get_rules() const override;

    void prep_counts() override;
    void sum_stats(bool) override;
    void show_stats() override;
    void reset_stats() override;

    bool counts_need_prep() const override
    { return true; }

    Usage get_usage() const override
    { return GLOBAL; }

    void set_trace(const snort::Trace*) const override;
    const snort::TraceOption* get_trace_options() const override;

private:
    StreamModuleConfig config;
    StreamReloadResourceManager reload_resource_manager;
    HPQReloadTuner hpq_rrt;
};

extern void base_prep();
extern void base_sum();
extern void base_stats();
extern void base_reset();

#endif
