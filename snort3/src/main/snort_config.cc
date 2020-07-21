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

#include "snort_config.h"

#include <grp.h>
#include <pwd.h>
#include <syslog.h>

#include "detection/detect.h"
#include "detection/detection_engine.h"
#include "detection/fp_config.h"
#include "detection/fp_create.h"
#include "filters/detection_filter.h"
#include "filters/rate_filter.h"
#include "filters/sfrf.h"
#include "filters/sfthreshold.h"
#include "flow/ha_module.h"
#include "hash/xhash.h"
#include "helpers/process.h"
#include "latency/latency_config.h"
#include "log/messages.h"
#include "managers/action_manager.h"
#include "managers/event_manager.h"
#include "managers/inspector_manager.h"
#include "managers/ips_manager.h"
#include "managers/module_manager.h"
#include "managers/mpse_manager.h"
#include "managers/plugin_manager.h"
#include "managers/so_manager.h"
#include "memory/memory_config.h"
#include "packet_io/sfdaq.h"
#include "packet_io/sfdaq_config.h"
#include "parser/parser.h"
#include "parser/vars.h"
#include "ports/rule_port_tables.h"
#include "profiler/profiler.h"
#include "protocols/packet.h"
#include "sfip/sf_ip.h"
#include "target_based/host_attributes.h"
#include "target_based/snort_protocols.h"
#include "trace/trace_config.h"
#include "utils/dnet_header.h"
#include "utils/util.h"
#include "utils/util_cstring.h"

#include "analyzer.h"
#include "thread_config.h"

using namespace snort;

#define LOG_NONE    "none"
#define LOG_DUMP    "dump"
#define LOG_CODECS  "codecs"

#define ALERT_NONE  "none"
#define ALERT_CMG   "cmg"
#define ALERT_JH    "jh"
#define ALERT_DJR   "djr"
#define ALERT_U2    "u2"
#define ALERT_AJK   "ajk"

#define OUTPUT_U2   "unified2"
#define OUTPUT_FAST "alert_fast"

static THREAD_LOCAL const SnortConfig* snort_conf = nullptr;

uint32_t SnortConfig::warning_flags = 0;

static std::vector<ScratchAllocator*> scratch_handlers;

//-------------------------------------------------------------------------
// private implementation
//-------------------------------------------------------------------------

static PolicyMode init_policy_mode(const SnortConfig* sc, PolicyMode mode)
{
    switch ( mode )
    {
    case POLICY_MODE__PASSIVE:
        if ( sc->adaptor_inline_test_mode() )
            return POLICY_MODE__INLINE_TEST;
        break;

    case POLICY_MODE__INLINE:
        if ( sc->adaptor_inline_test_mode() )
            return POLICY_MODE__INLINE_TEST;

        else if (!sc->adaptor_inline_mode())
        {
            ParseWarning(WARN_DAQ, "adapter is in passive mode; switching policy mode to tap.");
            return POLICY_MODE__PASSIVE;
        }
        break;

    case POLICY_MODE__INLINE_TEST:
        break;

    case POLICY_MODE__MAX:
        if ( sc->adaptor_inline_mode() )
            return POLICY_MODE__INLINE;
        else
            return POLICY_MODE__PASSIVE;
        break;
    }
    return mode;
}

static void init_policies(SnortConfig* sc)
{
    IpsPolicy* ips_policy = nullptr;
    InspectionPolicy* inspection_policy = nullptr;

    for ( unsigned idx = 0; idx <  sc->policy_map->ips_policy_count(); ++idx )
    {
        ips_policy = sc->policy_map->get_ips_policy(idx);
        ips_policy->policy_mode = init_policy_mode(sc, ips_policy->policy_mode);
    }

    for ( unsigned idx = 0; idx < sc->policy_map->inspection_policy_count(); ++idx )
    {
        inspection_policy = sc->policy_map->get_inspection_policy(idx);
        inspection_policy->policy_mode = init_policy_mode(sc, inspection_policy->policy_mode);
    }
}

void SnortConfig::init(const SnortConfig* const other_conf, ProtocolReference* protocol_reference)
{
    homenet.clear();
    obfuscation_net.clear();

    if ( !other_conf )
    {
        num_layers = DEFAULT_LAYERMAX;

        max_attribute_hosts = DEFAULT_MAX_ATTRIBUTE_HOSTS;
        max_attribute_services_per_host = DEFAULT_MAX_ATTRIBUTE_SERVICES_PER_HOST;

        max_metadata_services = DEFAULT_MAX_METADATA_SERVICES;
        mpls_stack_depth = DEFAULT_LABELCHAIN_LENGTH;

        daq_config = new SFDAQConfig();
        ActionManager::new_config(this);
        InspectorManager::new_config(this);

        num_slots = 0;
        state = nullptr;

        profiler = new ProfilerConfig;
        latency = new LatencyConfig();
        memory = new MemoryConfig();
        policy_map = new PolicyMap;
        thread_config = new ThreadConfig();
        global_dbus = new DataBus();

        memset(evalOrder, 0, sizeof(evalOrder));
        proto_ref = new ProtocolReference(protocol_reference);
        so_rules = new SoRules;
        trace_config = new TraceConfig;
    }
    else
    {
        clone(other_conf);
        policy_map = new PolicyMap(other_conf->policy_map);
    }
}

//-------------------------------------------------------------------------
// public methods
//-------------------------------------------------------------------------

/* A lot of this initialization can be skipped if not running in IDS mode
 * but the goal is to minimize config checks at run time when running in
 * IDS mode so we keep things simple and enforce that the only difference
 * among run_modes is how we handle packets via the log_func. */
SnortConfig::SnortConfig(const SnortConfig* const other_conf)
{
    init(other_conf, nullptr);
}

//  Copy the ProtocolReference data into the new SnortConfig.
SnortConfig::SnortConfig(ProtocolReference* protocol_reference)
{
    init(nullptr, protocol_reference);
}

SnortConfig::~SnortConfig()
{
    if ( cloned )
    {
        delete global_dbus;
        policy_map->set_cloned(true);
        delete policy_map;
        return;
    }

    for ( auto ct : classifications )
        delete ct.second;

    for ( auto rs : references )
        delete rs.second;

    for ( auto* s : scratchers )
        s->cleanup(this);

    FreeRuleLists(this);
    PortTablesFree(port_tables);

    ThresholdConfigFree(threshold_config);
    RateFilter_ConfigFree(rate_filter_config);
    DetectionFilterConfigFree(detection_filter_config);

    if ( event_queue_config )
        EventQueueConfigFree(event_queue_config);

    fpDeleteFastPacketDetection(this);
    OtnLookupFree(otn_map);

    delete rtn_hash_table;

    if (eth_dst )
        snort_free(eth_dst);

    if ( var_list )
        FreeVarList(var_list);

    if ( fast_pattern_config &&
        (!snort_conf || this == snort_conf ||
        (fast_pattern_config->get_search_api() !=
        get_conf()->fast_pattern_config->get_search_api())) )
    {
        MpseManager::stop_search_engine(fast_pattern_config->get_search_api());
    }
    delete fast_pattern_config;

    delete policy_map;
    InspectorManager::delete_config(this);
    ActionManager::delete_config(this);

    delete[] state;
    delete thread_config;
    delete trace_config;
    delete overlay_trace_config;
    delete ha_config;
    delete global_dbus;

    delete profiler;
    delete latency;
    delete memory;
    delete daq_config;
    delete proto_ref;
    delete so_rules;
    if ( plugins )
        delete plugins;
    reload_tuners.clear();

    trim_heap();
}

void SnortConfig::setup()
{
    if ( output_use_utc() )
        thiszone = 0;

    else
        thiszone = gmt2local(0);

    init_policies(this);
    ParseRules(this);
    OrderRuleLists(this);

    if ( rule_states )
    {
        rule_states->apply(this);
        delete rule_states;
        rule_states = nullptr;
    }

    ShowPolicyStats(this);

    /* Need to do this after dynamic detection stuff is initialized, too */
    IpsManager::verify(this);
    ModuleManager::load_commands(policy_map->get_shell());

    fpCreateFastPacketDetection(this);
}

void SnortConfig::post_setup()
{
    unsigned int handler_count = scratch_handlers.size();

    // Ensure we have allocated the scratch space vector for each thread
    for ( unsigned i = 0; i < num_slots; ++i )
        state[i].resize(handler_count);

    for ( auto* s : scratch_handlers )
    {
        if ( s and s->setup(this) )
            scratchers.push_back(s);
    }
}

void SnortConfig::clone(const SnortConfig* const conf)
{
    *this = *conf;
    global_dbus = new DataBus();
    if (conf->homenet.get_family() != 0)
        memcpy(&homenet, &conf->homenet, sizeof(homenet));

    if (conf->obfuscation_net.get_family() != 0)
        memcpy(&obfuscation_net, &conf->obfuscation_net, sizeof(obfuscation_net));
}

// merge in everything from the command line config
void SnortConfig::merge(SnortConfig* cmd_line)
{
    if ( !cmd_line->log_dir.empty() )
        log_dir = cmd_line->log_dir;

    if ( log_dir.empty() )
        log_dir = DEFAULT_LOG_DIR;

    run_prefix = cmd_line->run_prefix;
    id_offset = cmd_line->id_offset;
    id_subdir = cmd_line->id_subdir;
    id_zero = cmd_line->id_zero;

    /* Used because of a potential chroot */
    orig_log_dir = log_dir;
    event_log_id = cmd_line->event_log_id;

    run_flags |= cmd_line->run_flags;
    output_flags |= cmd_line->output_flags;
    logging_flags |= cmd_line->logging_flags;

    include_path = cmd_line->include_path;
    stdin_rules = cmd_line->stdin_rules;

    // only set by cmd_line to override other conf output settings
    output = cmd_line->output;

    /* Merge checksum flags.  If command line modified them, use from the
     * command line, else just use from config_file. */

    int cl_chk = cmd_line->policy_map->get_network_policy()->checksum_eval;
    int cl_drop = cmd_line->policy_map->get_network_policy()->checksum_drop;

    NetworkPolicy* nw_policy = nullptr;

    for ( unsigned idx = 0; idx < policy_map->network_policy_count(); ++idx )
    {
        nw_policy = policy_map->get_network_policy(idx);

        if ( !(cl_chk & CHECKSUM_FLAG__DEF) )
            nw_policy->checksum_eval = cl_chk;

        if ( !(cl_drop & CHECKSUM_FLAG__DEF) )
            nw_policy->checksum_drop = cl_drop;
    }

    /* FIXIT-L do these belong in network policy? */
    if (cmd_line->num_layers != 0)
        num_layers = cmd_line->num_layers;

    if (cmd_line->max_ip6_extensions != 0)
        max_ip6_extensions = cmd_line->max_ip6_extensions;

    if (cmd_line->max_ip_layers != 0)
        max_ip_layers = cmd_line->max_ip_layers;

    if (cmd_line->obfuscation_net.get_family() != 0)
        memcpy(&obfuscation_net, &cmd_line->obfuscation_net, sizeof(obfuscation_net));

    if (cmd_line->homenet.get_family() != 0)
        memcpy(&homenet, &cmd_line->homenet, sizeof(homenet));

    if ( !cmd_line->bpf_file.empty() )
        bpf_file = cmd_line->bpf_file;

    if ( !cmd_line->bpf_filter.empty() )
        bpf_filter = cmd_line->bpf_filter;

    if (cmd_line->pkt_cnt != 0)
        pkt_cnt = cmd_line->pkt_cnt;

    if (cmd_line->pkt_skip != 0)
        pkt_skip = cmd_line->pkt_skip;

    if (cmd_line->pkt_pause_cnt != 0)
        pkt_pause_cnt = cmd_line->pkt_pause_cnt;

    if (cmd_line->group_id != -1)
        group_id = cmd_line->group_id;

    if (cmd_line->user_id != -1)
        user_id = cmd_line->user_id;

    /* Only configurable on command line */
    if (cmd_line->file_mask != 0)
        file_mask = cmd_line->file_mask;

    if ( !cmd_line->chroot_dir.empty() )
        chroot_dir = cmd_line->chroot_dir;

    if ( cmd_line->dirty_pig )
        dirty_pig = cmd_line->dirty_pig;

    if ( !cmd_line->metadata_filter.empty() )
        metadata_filter = cmd_line->metadata_filter;

    daq_config->overlay(cmd_line->daq_config);

    if (cmd_line->mpls_stack_depth != DEFAULT_LABELCHAIN_LENGTH)
        mpls_stack_depth = cmd_line->mpls_stack_depth;

    /* Set MPLS payload type here if it hasn't been defined */
    if ((cmd_line->mpls_payload_type == 0) &&
        (mpls_payload_type == 0))
    {
        mpls_payload_type = DEFAULT_MPLS_PAYLOADTYPE;
    }
    else if (cmd_line->mpls_payload_type != 0)
    {
        mpls_payload_type = cmd_line->mpls_payload_type;
    }

    if (cmd_line->run_flags & RUN_FLAG__PROCESS_ALL_EVENTS)
        event_queue_config->process_all_events = 1;

#ifdef SHELL
    if ( cmd_line->remote_control_port )
        remote_control_port = cmd_line->remote_control_port;
    else if ( !cmd_line->remote_control_socket.empty() )
        remote_control_socket = cmd_line->remote_control_socket;
#endif

    // config file vars are stored differently
    // FIXIT-M should cmd_line use the same var list / table?
    var_list = nullptr;

    assert(!state);
    num_slots = offload_threads + ThreadConfig::get_instance_max();
    state = new std::vector<void*>[num_slots];
}

bool SnortConfig::verify() const
{
    bool config_ok = false;
    const SnortConfig* sc = get_conf();

    if ( sc->asn1_mem != asn1_mem )
        ReloadError("Changing detection.asn1_mem requires a restart.\n");

    else if ( sc->bpf_filter != bpf_filter )
        ReloadError("Changing packets.bfp_filter requires a restart.\n");

    else if ( sc->respond_attempts != respond_attempts )
        ReloadError("Changing active.attempts requires a restart.\n");

    else if (  sc->respond_device != respond_device )
        ReloadError("Changing active.device requires a restart.\n");

    else if (sc->chroot_dir != chroot_dir)
        ReloadError("Changing process.chroot requires a restart.\n");

    else if ((sc->run_flags & RUN_FLAG__DAEMON) != (run_flags & RUN_FLAG__DAEMON))
        ReloadError("Changing process.daemon requires a restart.\n");

    else if (sc->orig_log_dir != orig_log_dir)
        ReloadError("Changing output.logdir requires a restart.\n");

    else if (sc->group_id != group_id)
        ReloadError("Changing process.setgid requires a restart.\n");

    else if (sc->user_id != user_id)
        ReloadError("Changing process.setuid requires a restart.\n");

    else if (sc->daq_config->get_mru_size() != daq_config->get_mru_size())
        ReloadError("Changing daq.snaplen requires a restart.\n");

    else if (sc->threshold_config->memcap != threshold_config->memcap)
        ReloadError("Changing alerts.event_filter_memcap requires a restart.\n");

    else  if (sc->rate_filter_config->memcap != rate_filter_config->memcap)
        ReloadError("Changing alerts.rate_filter_memcap requires a restart.\n");

    else if (sc->detection_filter_config->memcap != detection_filter_config->memcap)
        ReloadError("Changing alerts.detection_filter_memcap requires a restart.\n");

    else
        config_ok = true;

    return config_ok;
}

void SnortConfig::set_alert_before_pass(bool enabled)
{
    if (enabled)
        run_flags |= RUN_FLAG__ALERT_BEFORE_PASS;
    else
        run_flags &= ~RUN_FLAG__ALERT_BEFORE_PASS;
}

void SnortConfig::set_chroot_dir(const char* directory)
{
    if (directory)
        chroot_dir = directory;
    else
        chroot_dir.clear();
}

void SnortConfig::set_create_pid_file(bool enabled)
{
    if (enabled)
        run_flags |= RUN_FLAG__CREATE_PID_FILE;
    else
        run_flags &= ~RUN_FLAG__CREATE_PID_FILE;
}

void SnortConfig::set_daemon(bool enabled)
{
    if (enabled)
    {
        run_flags |= RUN_FLAG__DAEMON;
    }
    else
        run_flags &= ~RUN_FLAG__DAEMON;
}

void SnortConfig::set_decode_data_link(bool enabled)
{
    if (enabled)
    {
        output_flags |= OUTPUT_FLAG__SHOW_DATA_LINK;
    }
    else
        output_flags &= ~OUTPUT_FLAG__SHOW_DATA_LINK;
}

void SnortConfig::set_dump_chars_only(bool enabled)
{
    if (enabled)
    {
        /* dump the application layer as text only */
        output_flags |= OUTPUT_FLAG__CHAR_DATA;
    }
    else
        output_flags &= ~OUTPUT_FLAG__CHAR_DATA;
}

void SnortConfig::set_dump_payload(bool enabled)
{
    if (enabled)
    {
        /* dump the application layer */
        output_flags |= OUTPUT_FLAG__APP_DATA;
    }
    else
        output_flags &= ~OUTPUT_FLAG__APP_DATA;
}

void SnortConfig::set_dump_payload_verbose(bool enabled)
{
    if (enabled)
    {
        output_flags |= OUTPUT_FLAG__VERBOSE_DUMP;
    }
    else
        output_flags &= ~OUTPUT_FLAG__VERBOSE_DUMP;
}

void SnortConfig::set_dst_mac(const char* mac_addr)
{
    if (mac_addr)
    {
        eth_addr_t dst;

        if (eth_pton(mac_addr, &dst) < 0)
        {
            ParseError("Format check failed: %s,  Use format like 12:34:56:78:90:1a", mac_addr);
            return;
        }
        snort_free(eth_dst);
        eth_dst = (uint8_t*)snort_calloc(sizeof(dst.data));
        memcpy(eth_dst, dst.data, sizeof(dst.data));
    }
    else
    {
        snort_free(eth_dst);
        eth_dst = nullptr;
    }
}

void SnortConfig::set_log_dir(const char* directory)
{
    if (directory)
        log_dir = directory;
    else
        log_dir.clear();
}

void SnortConfig::set_dirty_pig(bool enabled)
{
    dirty_pig = enabled;
}

void SnortConfig::set_obfuscate(bool enabled)
{
    if (enabled)
        output_flags |= OUTPUT_FLAG__OBFUSCATE;
    else
        output_flags &= ~OUTPUT_FLAG__OBFUSCATE;
}

void SnortConfig::set_no_logging_timestamps(bool enabled)
{
    if (enabled)
        output_flags |= OUTPUT_FLAG__NO_TIMESTAMP;
    else
        output_flags &= ~OUTPUT_FLAG__NO_TIMESTAMP;
}

void SnortConfig::set_obfuscation_mask(const char* mask)
{
    if (!mask)
        return;

    output_flags |= OUTPUT_FLAG__OBFUSCATE;

    obfuscation_net.set(mask);
}

void SnortConfig::set_quiet(bool enabled)
{
    if (enabled)
        logging_flags |= LOGGING_FLAG__QUIET;
    else
        logging_flags &= ~LOGGING_FLAG__QUIET;
}

void SnortConfig::set_gid(const char* args)
{
    struct group* gr;
    long target_gid;
    char* endptr;

    if (!args)
        return;

    target_gid = SnortStrtol(args, &endptr, 10);
    if (*endptr != '\0')
        gr = getgrnam(args); // main thread only
    else if (errno == ERANGE || target_gid < 0)
    {
        ParseError("group id '%s' out of range.", args);
        return;
    }
    else
        gr = getgrgid((gid_t)target_gid);  // main thread only

    if (!gr)
    {
        ParseError("group '%s' unknown.", args);
        return;
    }

    /* If we're already running as the desired group ID, don't bother to try changing it later. */
    if (gr->gr_gid != getgid())
        group_id = (int)gr->gr_gid;
}

void SnortConfig::set_uid(const char* args)
{
    struct passwd* pw;
    long target_uid;
    char* endptr;

    if (!args)
        return;

    target_uid = SnortStrtol(args, &endptr, 10);
    if (*endptr != '\0')
        pw = getpwnam(args); // main thread only
    else if (errno == ERANGE || target_uid < 0)
    {
        ParseError("user id '%s' out of range.", args);
        return;
    }
    else
        pw = getpwuid((uid_t)target_uid);  // main thread only

    if (!pw)
    {
        ParseError("user '%s' unknown.", args);
        return;
    }

    /* Set group ID to user's default group if not already set.
       If we're already running as the desired user and/or group ID,
       don't bother to try changing it later. */
    if (pw->pw_uid != getuid())
        user_id = (int)pw->pw_uid;

    if (group_id == -1 && pw->pw_gid != getgid())
        group_id = (int)pw->pw_gid;
}

void SnortConfig::set_show_year(bool enabled)
{
    if (enabled)
    {
        output_flags |= OUTPUT_FLAG__INCLUDE_YEAR;
    }
    else
        output_flags &= ~OUTPUT_FLAG__INCLUDE_YEAR;
}

void SnortConfig::set_treat_drop_as_alert(bool enabled)
{
    if (enabled)
        run_flags |= RUN_FLAG__TREAT_DROP_AS_ALERT;
    else
        run_flags &= ~RUN_FLAG__TREAT_DROP_AS_ALERT;
}

void SnortConfig::set_treat_drop_as_ignore(bool enabled)
{
    if (enabled)
        run_flags |= RUN_FLAG__TREAT_DROP_AS_IGNORE;
    else
        run_flags &= ~RUN_FLAG__TREAT_DROP_AS_IGNORE;
}

void SnortConfig::set_process_all_events(bool enabled)
{
    if (enabled)
        run_flags |= RUN_FLAG__PROCESS_ALL_EVENTS;
    else
        run_flags &= ~RUN_FLAG__PROCESS_ALL_EVENTS;
}

#ifdef ACCESSPERMS
# define FILE_ACCESS_BITS ACCESSPERMS
#else
# ifdef S_IAMB
#  define FILE_ACCESS_BITS S_IAMB
# else
#  define FILE_ACCESS_BITS 0x1FF
# endif
#endif

void SnortConfig::set_umask(uint32_t mask)
{
    file_mask = (mode_t)mask;
}

void SnortConfig::set_utc(bool enabled)
{
    if (enabled)
        output_flags |= OUTPUT_FLAG__USE_UTC;
    else
        output_flags &= ~OUTPUT_FLAG__USE_UTC;
}

void SnortConfig::set_verbose(bool enabled)
{
    if (enabled)
    {
        logging_flags |= LOGGING_FLAG__VERBOSE;
    }
    else
        logging_flags &= ~LOGGING_FLAG__VERBOSE;
}

void SnortConfig::set_overlay_trace_config(TraceConfig* tc)
{
    delete overlay_trace_config;
    overlay_trace_config = tc;
}

void SnortConfig::set_tunnel_verdicts(const char* args)
{
    char* tmp, * tok;

    tmp = snort_strdup(args);
    char* lasts = { nullptr };
    tok = strtok_r(tmp, " ,", &lasts);

    while (tok)
    {
        if (!strcasecmp(tok, "gtp"))
            tunnel_mask |= TUNNEL_GTP;

        else if (!strcasecmp(tok, "teredo"))
            tunnel_mask |= TUNNEL_TEREDO;

        else if (!strcasecmp(tok, "vxlan"))
            tunnel_mask |= TUNNEL_VXLAN;

        else if (!strcasecmp(tok, "6in4"))
            tunnel_mask |= TUNNEL_6IN4;

        else if (!strcasecmp(tok, "4in6"))
            tunnel_mask |= TUNNEL_4IN6;

        else if (!strcasecmp(tok, "4in4"))
            tunnel_mask |= TUNNEL_4IN4;

        else if (!strcasecmp(tok, "6in6"))
            tunnel_mask |= TUNNEL_6IN6;

        else if (!strcasecmp(tok, "gre"))
            tunnel_mask |= TUNNEL_GRE;

        else if (!strcasecmp(tok, "mpls"))
            tunnel_mask |= TUNNEL_MPLS;

        else
        {
            ParseError("unknown tunnel bypass protocol");
            return;
        }

        tok = strtok_r(nullptr, " ,", &lasts);
    }
    snort_free(tmp);
}

void SnortConfig::set_include_path(const char* path)
{
    if (path)
        include_path = path;
    else
        include_path.clear();
}

void SnortConfig::add_plugin_path(const char* path)
{
    if (!path)
        return;

    if (!plugin_path.empty())
        plugin_path += ":" + std::string(path);
    else
        plugin_path = path;
}

void SnortConfig::set_tweaks(const char* t)
{
    if (t)
        tweaks = t;
    else
        tweaks.clear();
}

void SnortConfig::add_script_path(const char* path)
{
    if (path)
        script_paths.emplace_back(path);
}

void SnortConfig::set_alert_mode(const char* val)
{
    if (strcasecmp(val, ALERT_NONE) == 0)
        EventManager::enable_alerts(false);

    else if ( !strcasecmp(val, ALERT_CMG) or !strcasecmp(val, ALERT_JH) or
        !strcasecmp(val, ALERT_DJR) )
    {
        output = OUTPUT_FAST;
        output_flags |= OUTPUT_FLAG__SHOW_DATA_LINK;
        output_flags |= OUTPUT_FLAG__APP_DATA;
    }
    else if ( !strcasecmp(val, ALERT_U2) or !strcasecmp(val, ALERT_AJK) )
        output = OUTPUT_U2;

    else
        output = val;

    output_flags |= OUTPUT_FLAG__ALERTS;
    Analyzer::set_main_hook(DetectionEngine::inspect);
}

void SnortConfig::set_log_mode(const char* val)
{
    if (strcasecmp(val, LOG_NONE) == 0)
    {
        Analyzer::set_main_hook(snort_ignore);
        EventManager::enable_logs(false);
    }
    else
    {
        if ( !strcmp(val, LOG_DUMP) )
            val = LOG_CODECS;
        output = val;
        Analyzer::set_main_hook(snort_log);
    }
}

void SnortConfig::enable_syslog()
{
    static bool syslog_configured = false;

    if (syslog_configured)
        return;

    openlog("snort", LOG_PID | LOG_CONS, LOG_DAEMON);

    logging_flags |= LOGGING_FLAG__SYSLOG;
    syslog_configured = true;
}

bool SnortConfig::get_default_rule_state() const
{
    switch ( get_ips_policy()->default_rule_state )
    {
    case IpsPolicy::INHERIT_ENABLE:
        return global_default_rule_state;

    case IpsPolicy::ENABLED:
        return true;

    case IpsPolicy::DISABLED:
        return false;
    }
    return true;
}

bool SnortConfig::tunnel_bypass_enabled(uint16_t proto) const
{
    return !((tunnel_mask & proto) or SFDAQ::get_tunnel_bypass(proto));
}

int SnortConfig::request_scratch(ScratchAllocator* s)
{
    scratch_handlers.emplace_back(s);

    // We return an index that the caller uses to reference their per thread
    // scratch space
    return scratch_handlers.size() - 1;
}

void SnortConfig::release_scratch(int id)
{
    assert((unsigned)id < scratch_handlers.size());
    scratch_handlers[id] = nullptr;
}

SnortConfig* SnortConfig::get_main_conf()
{ return const_cast<SnortConfig*>(snort_conf); }

const SnortConfig* SnortConfig::get_conf()
{ return snort_conf; }

void SnortConfig::set_conf(const SnortConfig* sc)
{
    snort_conf = sc;

    if ( sc )
    {
        Shell* sh = sc->policy_map->get_shell(0);
        if (sc->policy_map->get_policies(sh))
            set_policies(sc, sh);
    }
}

void SnortConfig::cleanup_fatal_error()
{
    // guard against FatalError calls from packet threads (which should not happen!)
    // FIXIT-L should also guard against non-main non-packet threads calling FatalError
    if ( is_packet_thread() )
        return;

    const SnortConfig* sc = SnortConfig::get_conf();
    if ( sc && !sc->dirty_pig )
    {
        ModuleManager::term();
        EventManager::release_plugins();
        IpsManager::release_plugins();
        InspectorManager::release_plugins();
    }
}

