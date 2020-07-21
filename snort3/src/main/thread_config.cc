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
// thread_config.cc author Michael Altizer <mialtize@cisco.com>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "thread_config.h"

#include <hwloc.h>

#include "log/messages.h"
#include "utils/util.h"

#ifdef UNIT_TEST
#include "catch/snort_catch.h"
#endif

using namespace snort;

static hwloc_topology_t topology = nullptr;
static hwloc_cpuset_t process_cpuset = nullptr;
static const struct hwloc_topology_support* topology_support = nullptr;
static unsigned instance_max = 1;

struct CpuSet
{
    CpuSet(hwloc_cpuset_t set) : cpuset(set) { }
    ~CpuSet()
    {
        if (cpuset)
            hwloc_bitmap_free(cpuset);
    }

    hwloc_cpuset_t cpuset;
};

bool ThreadConfig::init()
{
    if (hwloc_topology_init(&topology))
        return false;
    if (hwloc_topology_load(topology))
    {
        hwloc_topology_destroy(topology);
        return false;
    }
    topology_support = hwloc_topology_get_support(topology);
    if (topology_support->cpubind->get_thisproc_cpubind)
    {
        process_cpuset = hwloc_bitmap_alloc();
        hwloc_get_cpubind(topology, process_cpuset, HWLOC_CPUBIND_PROCESS);
    }
    else
        process_cpuset = hwloc_bitmap_dup(hwloc_topology_get_allowed_cpuset(topology));
    return true;
}

void ThreadConfig::set_instance_max(unsigned max)
{
    if (max)
        instance_max = max;
    else
    {
        /* A max of 0 indicates automatic allocation.  Set the instance max to the total number of
            CPUs in the our process's running cpuset. */
        instance_max = hwloc_bitmap_weight(process_cpuset);
    }
}

unsigned ThreadConfig::get_instance_max()
{
    return instance_max;
}

CpuSet* ThreadConfig::validate_cpuset_string(const char* cpuset_str)
{
    hwloc_bitmap_t cpuset = hwloc_bitmap_alloc();
    if (hwloc_bitmap_list_sscanf(cpuset, cpuset_str) ||
            !hwloc_bitmap_isincluded(cpuset, process_cpuset))
    {
        hwloc_bitmap_free(cpuset);
        return nullptr;
    }
    return new CpuSet(cpuset);
}

void ThreadConfig::destroy_cpuset(CpuSet *cpuset)
{
    delete cpuset;
}

void ThreadConfig::term()
{
    if (topology)
    {
        hwloc_topology_destroy(topology);
        topology = nullptr;
    }
    if (process_cpuset)
    {
        hwloc_bitmap_free(process_cpuset);
        process_cpuset = nullptr;
    }
    topology_support = nullptr;
}

ThreadConfig::~ThreadConfig()
{
    for (auto& iter : thread_affinity)
        delete iter.second;

    for (auto& iter : named_thread_affinity)
        delete iter.second;
}

void ThreadConfig::set_thread_affinity(SThreadType type, unsigned id, CpuSet* cpuset)
{
    if (topology_support->cpubind->set_thisthread_cpubind)
    {
        TypeIdPair key { type, id };

        auto iter = thread_affinity.find(key);
        if (iter != thread_affinity.end())
            delete iter->second;
        thread_affinity[key] = cpuset;
    }
    else
        ParseWarning(WARN_CONF, "This platform does not support setting thread affinity.\n");
}

void ThreadConfig::set_named_thread_affinity(const std::string& name, CpuSet* cpuset)
{
    if (topology_support->cpubind->set_thisthread_cpubind)
    {
        auto iter = named_thread_affinity.find(name);
        if (iter != named_thread_affinity.end())
            delete iter->second;
        named_thread_affinity[name] = cpuset;
    }
    else
        ParseWarning(WARN_CONF, "This platform does not support setting thread affinity.\n");
}

void ThreadConfig::implement_thread_affinity(SThreadType type, unsigned id)
{
    if (!topology_support->cpubind->set_thisthread_cpubind)
        return;

    TypeIdPair key { type, id };
    hwloc_cpuset_t current_cpuset, desired_cpuset;
    char* s;

    auto iter = thread_affinity.find(key);
    if (iter != thread_affinity.end())
        desired_cpuset = iter->second->cpuset;
    else
        desired_cpuset = process_cpuset;

    // A data race in this library function ensues from the usage of a static variable
    // to dump chars when calculating string length. This does not affect functionality.
    hwloc_bitmap_list_asprintf(&s, desired_cpuset);

    current_cpuset = hwloc_bitmap_alloc();
    hwloc_get_cpubind(topology, current_cpuset, HWLOC_CPUBIND_THREAD);
    if (!hwloc_bitmap_isequal(current_cpuset, desired_cpuset))
        LogMessage("Binding thread %u (type %u) to %s.\n", id, type, s);
    hwloc_bitmap_free(current_cpuset);

    if (hwloc_set_cpubind(topology, desired_cpuset, HWLOC_CPUBIND_THREAD))
    {
        FatalError("Failed to pin thread %u (type %u) to %s: %s (%d)\n",
            id, type, s, get_error(errno), errno);
    }

    free(s);
}

void ThreadConfig::implement_named_thread_affinity(const std::string& name)
{
    if (!topology_support->cpubind->set_thisthread_cpubind)
        return;

    auto iter = named_thread_affinity.find(name);
    if (iter != named_thread_affinity.end())
    {
        char* s;

        auto desired_cpuset = iter->second->cpuset;
        hwloc_bitmap_list_asprintf(&s, desired_cpuset);

        auto current_cpuset = hwloc_bitmap_alloc();
        hwloc_get_cpubind(topology, current_cpuset, HWLOC_CPUBIND_THREAD);
        if (!hwloc_bitmap_isequal(current_cpuset, desired_cpuset))
            LogMessage("Binding thread %s to %s.\n", name.c_str(), s);
        hwloc_bitmap_free(current_cpuset);

        if (hwloc_set_cpubind(topology, desired_cpuset, HWLOC_CPUBIND_THREAD))
        {
            FatalError("Failed to pin thread %s to %s: %s (%d)\n",
                name.c_str(), s, get_error(errno), errno);
        }

        free(s);
    }
    else
        implement_thread_affinity(get_thread_type(), DEFAULT_THREAD_ID);
}


// -----------------------------------------------------------------------------
// unit tests
// -----------------------------------------------------------------------------

#ifdef UNIT_TEST

TEST_CASE("Parse cpuset string negative test", "[ThreadConfig]")
{
    CpuSet* cpuset = ThreadConfig::validate_cpuset_string("totally a bad cpuset string");
    CHECK(cpuset == nullptr);
}

TEST_CASE("Parse cpuset string positive test", "[ThreadConfig]")
{
    char* s;
    hwloc_bitmap_list_asprintf(&s, process_cpuset);
    CpuSet* cpuset = ThreadConfig::validate_cpuset_string(s);
    free(s);
    CHECK(cpuset != nullptr);
    ThreadConfig::destroy_cpuset(cpuset);
}

TEST_CASE("Set and check max packet threads", "[ThreadConfig]")
{
    CHECK(ThreadConfig::get_instance_max() == 1);
    unsigned new_max = hwloc_bitmap_weight(process_cpuset) * 2;
    ThreadConfig::set_instance_max(new_max);
    CHECK(ThreadConfig::get_instance_max() == new_max);
    ThreadConfig::set_instance_max(0);
    CHECK(ThreadConfig::get_instance_max() == hwloc_bitmap_weight(process_cpuset));
}

TEST_CASE("Set and implement thread affinity", "[ThreadConfig]")
{
    if (topology_support->cpubind->set_thisthread_cpubind)
    {
        CpuSet* cpuset = new CpuSet(hwloc_bitmap_dup(process_cpuset));
        CpuSet* cpuset2 = new CpuSet(hwloc_bitmap_dup(process_cpuset));
        ThreadConfig tc;

        hwloc_bitmap_singlify(cpuset->cpuset);
        tc.set_thread_affinity(STHREAD_TYPE_PACKET, 0, cpuset2);
        tc.set_thread_affinity(STHREAD_TYPE_PACKET, 0, cpuset);
        tc.implement_thread_affinity(STHREAD_TYPE_PACKET, 0);

        hwloc_cpuset_t thread_cpuset = hwloc_bitmap_alloc();
        hwloc_get_cpubind(topology, thread_cpuset, HWLOC_CPUBIND_THREAD);
        CHECK(hwloc_bitmap_isequal(thread_cpuset, cpuset->cpuset));

        tc.implement_thread_affinity(STHREAD_TYPE_MAIN, 0);
        hwloc_get_cpubind(topology, thread_cpuset, HWLOC_CPUBIND_THREAD);
        CHECK(hwloc_bitmap_isequal(thread_cpuset, process_cpuset));

        hwloc_bitmap_free(thread_cpuset);
    }
}

TEST_CASE("Named thread affinity configured", "[ThreadConfig]")
{
    if (topology_support->cpubind->set_thisthread_cpubind)
    {
        CpuSet* cpuset = new CpuSet(hwloc_bitmap_dup(process_cpuset));
        ThreadConfig tc;

        hwloc_cpuset_t thread_cpuset = hwloc_bitmap_alloc();

        // Configure named thread.
        hwloc_bitmap_singlify(cpuset->cpuset);
        tc.set_named_thread_affinity("found", cpuset);

        // The one in the named map, should have the specified cpuset.
        tc.implement_named_thread_affinity("found");
        hwloc_get_cpubind(topology, thread_cpuset, HWLOC_CPUBIND_THREAD);
        CHECK(hwloc_bitmap_isequal(thread_cpuset, cpuset->cpuset));

        // The one not in the named map, should have the process cpuset
        // if no type has been configured for it.
        tc.implement_named_thread_affinity("not found, no type configured");
        hwloc_get_cpubind(topology, thread_cpuset, HWLOC_CPUBIND_THREAD);
        CHECK(hwloc_bitmap_isequal(thread_cpuset, process_cpuset));

        hwloc_bitmap_free(thread_cpuset);
    }
}

TEST_CASE("Named thread affinity with type configured", "[ThreadConfig]")
{
    if (topology_support->cpubind->set_thisthread_cpubind)
    {
        CpuSet* type_cpuset = new CpuSet(hwloc_bitmap_dup(process_cpuset));
        ThreadConfig tc;

        hwloc_cpuset_t thread_cpuset = hwloc_bitmap_alloc();

        // Configure type affinity, but not the named thread affinity.
        hwloc_bitmap_singlify(type_cpuset->cpuset);
        tc.set_thread_affinity(STHREAD_TYPE_MAIN, ThreadConfig::DEFAULT_THREAD_ID, type_cpuset);

        // The named thread should inherit the type affinity.
        tc.implement_named_thread_affinity("not found, type other");
        hwloc_get_cpubind(topology, thread_cpuset, HWLOC_CPUBIND_THREAD);
        CHECK(hwloc_bitmap_isequal(thread_cpuset, type_cpuset->cpuset));

        hwloc_bitmap_free(thread_cpuset);
    }
}

#endif
