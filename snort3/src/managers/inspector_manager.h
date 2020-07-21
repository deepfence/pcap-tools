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
// inspector_manager.h author Russ Combs <rucombs@cisco.com>

#ifndef INSPECTOR_MANAGER_H
#define INSPECTOR_MANAGER_H

// Factory for Inspectors.
// Also provides packet evaluation.

#include "framework/inspector.h"

class Binder;
struct FrameworkPolicy;
struct InspectionPolicy;

namespace snort
{
struct Packet;
struct SnortConfig;

//-------------------------------------------------------------------------

class InspectorManager
{
public:
    static void add_plugin(const InspectApi* api);
    static void dump_plugins();
    static void dump_buffers();
    static void release_plugins();

    static std::vector<const InspectApi*> get_apis();

    static void new_policy(InspectionPolicy*, InspectionPolicy*);
    static void delete_policy(InspectionPolicy*, bool cloned);
    static void update_policy(SnortConfig* sc);

    static void new_config(SnortConfig*);
    static void delete_config(SnortConfig*);

    static void instantiate(
        const InspectApi*, Module*, SnortConfig*, const char* name = nullptr);

    static bool delete_inspector(SnortConfig*, const char* iname);
    static void free_inspector(Inspector*);
    static InspectSsnFunc get_session(uint16_t proto);

    SO_PUBLIC static Inspector* get_inspector(
        const char* key, bool dflt_only = false, const SnortConfig* = nullptr);

    SO_PUBLIC static Inspector* get_inspector_by_service(const char*);

    SO_PUBLIC static Binder* get_binder();

    SO_PUBLIC static Inspector* acquire(const char* key, bool dflt_only = false);
    SO_PUBLIC static void release(Inspector*);

    static bool configure(SnortConfig*, bool cloned = false);
    static void prepare_controls(SnortConfig*);
    static void print_config(SnortConfig*);

    static void thread_init(const SnortConfig*);
    static void thread_reinit(const SnortConfig*);

    static void thread_stop(const SnortConfig*);
    static void thread_term();

    static void release_policy(FrameworkPolicy*);
    static void dispatch_meta(FrameworkPolicy*, int type, const uint8_t* data);

    static void execute(Packet*);
    static void probe(Packet*);

    static void clear(Packet*);
    static void empty_trash();
    static bool inspector_exists_in_any_policy(const char* key, SnortConfig* sc);

#ifdef PIGLET
    static Inspector* instantiate(const char*, Module*, SnortConfig*);
#endif

private:
    static void bumble(Packet*);
    template<bool T> static void full_inspection(Packet*);
    template<bool T> static void internal_execute(Packet*);
};
}
#endif

