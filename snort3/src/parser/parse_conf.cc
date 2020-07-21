//--------------------------------------------------------------------------
// Copyright (C) 2014-2020 Cisco and/or its affiliates. All rights reserved.
// Copyright (C) 2002-2013 Sourcefire, Inc.
// Copyright (C) 1998-2002 Martin Roesch <roesch@sourcefire.com>
// Copyright (C) 2000,2001 Andrew R. Baker <andrewb@uab.edu>
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

#include "parse_conf.h"

#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>

#include <fstream>
#include <stack>

#include "log/messages.h"
#include "main/snort_config.h"
#include "managers/action_manager.h"
#include "managers/module_manager.h"
#include "sfip/sf_vartable.h"
#include "target_based/snort_protocols.h"
#include "utils/util.h"

#include "config_file.h"
#include "parser.h"
#include "parse_stream.h"
#include "vars.h"

using namespace snort;

struct Location
{
    const char* code;
    std::string path;
    std::string file;
    unsigned line;

    Location(const char* c, const char* p, const char* f, unsigned u)
    { code = c; path = p; file = f; line = u; }
};

static std::stack<Location> files;
static int rules_file_depth = 0;

const char* get_parse_file()
{
    if ( !files.empty() )
        return files.top().path.c_str();

    return get_snort_conf();
}

void get_parse_location(const char*& file, unsigned& line)
{
    if ( files.empty() )
    {
        file = nullptr;
        line = 0;
        return;
    }
    Location& loc = files.top();
    file = loc.file.c_str();
    line = loc.line;
}

static void print_parse_file(const char* msg, Location& loc)
{
    if ( SnortConfig::get_conf()->show_file_codes() )
        LogMessage("%s %s:%s:\n", msg, (loc.code ? loc.code : "?"), loc.file.c_str());

    else
        LogMessage("%s %s:\n", msg, loc.file.c_str());
}

void push_parse_location(
    const char* code, const char* path, const char* file, unsigned line)
{
    if ( !path )
        return;

    if ( !file )
        file = path;

    Location loc(code, path, file, line);
    files.push(loc);
    print_parse_file("Loading", loc);
}

void pop_parse_location()
{
    if ( !files.empty() )
    {
        Location& loc = files.top();
        print_parse_file("Finished", loc);
        files.pop();
    }
}

void inc_parse_position()
{
    if ( files.empty() )
        return;
    Location& loc = files.top();
    ++loc.line;
}

static bool valid_file(const char* file, std::string& path)
{
    path += '/';
    path += file;

    struct stat s;
    return stat(path.c_str(), &s) == 0;
}

static bool relative_to_parse_dir(const char* file, std::string& path)
{
    if ( !path.length() )
        path = get_parse_file();
    size_t idx = path.rfind('/');
    if ( idx != std::string::npos )
        path.erase(idx);
    else
        path = ".";
    return valid_file(file, path);
}

static bool relative_to_config_dir(const char* file, std::string& path)
{
    path = get_snort_conf_dir();
    return valid_file(file, path);
}

static bool relative_to_include_dir(const char* file, std::string& path)
{
    path = SnortConfig::get_conf()->include_path;
    if ( !path.length() )
        return false;
    return valid_file(file, path);
}

const char* get_config_file(const char* arg, std::string& file)
{
    bool absolute = (arg[0] == '/');

    if ( absolute )
    {
        file = arg;
        return "A";
    }
    std::string hint = file;

    if ( relative_to_include_dir(arg, file) )
        return "I";

    file = hint;

    if ( relative_to_parse_dir(arg, file) )
        return "F";

    if ( relative_to_config_dir(arg, file) )
        return "C";

    return nullptr;
}

void parse_include(SnortConfig* sc, const char* arg)
{
    assert(arg);
    arg = ExpandVars(sc, arg);
    std::string file = !rules_file_depth ? get_ips_policy()->includer : get_parse_file();

    const char* code = get_config_file(arg, file);

    if ( !code )
    {
        ParseError("can't open %s\n", arg);
        return;
    }
    push_parse_location(code, file.c_str(), arg);
    parse_rules_file(sc, file.c_str());
    pop_parse_location();
}

void ParseIpVar(SnortConfig* sc, const char* var, const char* val)
{
    int ret;
    IpsPolicy* p = get_ips_policy();  // FIXIT-M double check, see below
    DisallowCrossTableDuplicateVars(sc, var, VAR_TYPE__IPVAR);

    if ((ret = sfvt_define(p->ip_vartable, var, val)) != SFIP_SUCCESS)
    {
        switch (ret)
        {
        case SFIP_ARG_ERR:
            ParseError("the following is not allowed: %s.", val);
            return;

        case SFIP_DUPLICATE:
            ParseWarning(WARN_VARS, "Var '%s' redefined.", var);
            break;

        case SFIP_CONFLICT:
            ParseError("negated IP ranges that are more general than "
                "non-negated ranges are not allowed. Consider "
                "inverting the logic in %s.", var);
            return;

        case SFIP_NOT_ANY:
            ParseError("!any is not allowed in %s.", var);
            return;

        default:
            ParseError("failed to parse the IP address: %s.", val);
            return;
        }
    }
}

void add_service_to_otn(SnortConfig* sc, OptTreeNode* otn, const char* svc_name)
{
    if ( !strcmp(svc_name, "file") and otn->sigInfo.services.empty() )
    {
        // well-known services supporting file_data
        // applies to both alert file and service:file rules
        add_service_to_otn(sc, otn, "ftp-data");
        add_service_to_otn(sc, otn, "netbios-ssn");
        add_service_to_otn(sc, otn, "http");
        add_service_to_otn(sc, otn, "pop3");
        add_service_to_otn(sc, otn, "imap");
        add_service_to_otn(sc, otn, "smtp");
        add_service_to_otn(sc, otn, "file");
        return;
    }

    if ( !strcmp(svc_name, "http") )
        add_service_to_otn(sc, otn, "http2");

    SnortProtocolId svc_id = sc->proto_ref->add(svc_name);

    for ( const auto& si : otn->sigInfo.services )
        if ( si.snort_protocol_id == svc_id )
            return;  // already added

    SignatureServiceInfo si(svc_name, svc_id);
    otn->sigInfo.services.emplace_back(si);
}

// only keep drop rules ...
// if we are inline (and can actually drop),
// or we are going to just alert instead of drop,
// or we are going to ignore session data instead of drop.
// the alert case is tested for separately with SnortConfig::treat_drop_as_alert().
static inline int keep_drop_rules(const SnortConfig* sc)
{
    return ( sc->inline_mode() or sc->adaptor_inline_mode() or sc->treat_drop_as_ignore() );
}

static inline int load_as_drop_rules(const SnortConfig* sc)
{
    return ( sc->inline_test_mode() || sc->adaptor_inline_test_mode() );
}

Actions::Type get_rule_type(const char* s)
{
    Actions::Type rt = Actions::get_type(s);

    if ( rt == Actions::NONE )
        rt = ActionManager::get_action_type(s);

    const SnortConfig* sc = SnortConfig::get_conf();

    switch ( rt )
    {
    case Actions::DROP:
    case Actions::BLOCK:
    case Actions::RESET:
        if ( sc->treat_drop_as_alert() )
            return Actions::ALERT;

        if ( keep_drop_rules(sc) || load_as_drop_rules(sc) )
            return rt;

        return Actions::NONE;

    case Actions::NONE:
        ParseError("unknown rule type '%s'", s);
        break;

    default:
        break;
    }
    return rt;
}

ListHead* get_rule_list(SnortConfig* sc, const char* s)
{
    const RuleListNode* p = sc->rule_lists;

    while ( p && strcmp(p->name, s) )
        p = p->next;

    return p ? p->RuleList : nullptr;
}

void parse_rules_file(SnortConfig* sc, const char* fname)
{
    if ( !fname )
        return;

    std::ifstream fs(fname, std::ios_base::binary);

    if ( !fs )
    {
        ParseError("unable to open rules file '%s': %s",
            fname, get_error(errno));
        return;
    }
    ++rules_file_depth;
    parse_stream(fs, sc);
    --rules_file_depth;
}

void parse_rules_string(SnortConfig* sc, const char* s)
{
    std::string rules = s;
    std::stringstream ss(rules);
    parse_stream(ss, sc);
}

