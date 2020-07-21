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
// shell.h author Russ Combs <rucombs@cisco.com>

#ifndef SHELL_H
#define SHELL_H

// Shell encapsulates a Lua state.  There is one for each policy file.

#include <set>
#include <stack>
#include <string>

struct lua_State;

namespace snort
{
struct SnortConfig;
}

class Shell
{
public:
    typedef std::set<std::string> Whitelist;

    Shell(const char* file = nullptr, bool load_defaults = false);
    ~Shell();

    void set_file(const char*);
    void set_overrides(const char*);
    void set_overrides(Shell*);

    bool configure(snort::SnortConfig*, bool is_fatal = true, bool is_root = false);
    void install(const char*, const struct luaL_Reg*);
    void execute(const char*, std::string&);

    const char* get_file() const
    { return file.c_str(); }

    const char* get_from() const
    { return parse_from.c_str(); }

    bool get_loaded() const
    { return loaded; }

public:
    static bool is_whitelisted(const std::string& key);
    static void whitelist_append(const char* keyword, bool is_prefix);

private:
    [[noreturn]] static int panic(lua_State*);
    static Shell* get_current_shell();

private:
    static std::string fatal;
    static std::stack<Shell*> current_shells;

private:
    void clear_whitelist()
    {
        whitelist.clear();
        internal_whitelist.clear();
        whitelist_prefixes.clear();
    }

    const Whitelist& get_whitelist() const
    { return whitelist; }

    const Whitelist& get_internal_whitelist() const
    { return internal_whitelist; }

    const Whitelist& get_whitelist_prefixes() const
    { return whitelist_prefixes; }

    void print_whitelist() const;
    void whitelist_update(const char* keyword, bool is_prefix);

private:
    bool loaded;
    bool bootstrapped = false;
    lua_State* lua;
    std::string file;
    std::string parse_from;
    std::string overrides;
    Whitelist whitelist;
    Whitelist internal_whitelist;
    Whitelist whitelist_prefixes;
};

#endif

