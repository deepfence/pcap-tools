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

// perf_reload_tuner.h author Michael Matirko <mmatirko@cisco.com>

#ifndef PERF_RELOAD_TUNER_H
#define PERF_RELOAD_TUNER_H

#include "main/snort_config.h"

class PerfMonReloadTuner : public snort::ReloadResourceTuner
{
public:
    PerfMonReloadTuner() = default;

    bool tinit() override;

    bool tune_idle_context() override
        { return tune_resources(max_work_idle); }

    bool tune_packet_context() override
        { return tune_resources(max_work); }

    bool tune_resources(unsigned work_limit);

    void set_memcap(size_t new_memcap)
        { memcap = new_memcap; }

    size_t get_memcap()
        { return memcap; }

private:
    size_t memcap = 0;

};

#endif

