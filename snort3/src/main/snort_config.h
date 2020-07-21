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

#ifndef SNORT_CONFIG_H
#define SNORT_CONFIG_H

// SnortConfig encapsulates all data loaded from the config files.
// FIXIT-L privatize most of this stuff.

#include <sys/types.h>

#include <list>
#include <unordered_map>
#include <vector>

#include "events/event_queue.h"
#include "framework/bits.h"
#include "helpers/scratch_allocator.h"
#include "main/policy.h"
#include "main/thread.h"
#include "sfip/sf_cidr.h"

#define DEFAULT_LOG_DIR "."

enum RunFlag
{
    RUN_FLAG__READ                = 0x00000001,
    RUN_FLAG__DAEMON              = 0x00000002,
    RUN_FLAG__DUMP_MSG_MAP        = 0x00000004,
    RUN_FLAG__DUMP_RULE_META      = 0x00000008,

    RUN_FLAG__INLINE              = 0x00000010,
    RUN_FLAG__STATIC_HASH         = 0x00000020,
    RUN_FLAG__CREATE_PID_FILE     = 0x00000040,
    RUN_FLAG__NO_LOCK_PID_FILE    = 0x00000080,

    RUN_FLAG__TREAT_DROP_AS_ALERT = 0x00000100,
    RUN_FLAG__ALERT_BEFORE_PASS   = 0x00000200,
    RUN_FLAG__CONF_ERROR_OUT      = 0x00000400,
    RUN_FLAG__MPLS_MULTICAST      = 0x00000800,

    RUN_FLAG__MPLS_OVERLAPPING_IP = 0x00001000,
    RUN_FLAG__PROCESS_ALL_EVENTS  = 0x00002000,
    RUN_FLAG__INLINE_TEST         = 0x00004000,
    RUN_FLAG__PCAP_SHOW           = 0x00008000,

    RUN_FLAG__SHOW_FILE_CODES     = 0x00010000,
    RUN_FLAG__PAUSE               = 0x00020000,
    RUN_FLAG__NO_PCRE             = 0x00040000,
    /* If stream is configured, the STATEFUL flag is set.  This is
     * somewhat misnamed and is used to assure a session is established */
    RUN_FLAG__ASSURE_EST          = 0x00080000,

    RUN_FLAG__TREAT_DROP_AS_IGNORE= 0x00100000,
    RUN_FLAG__DUMP_RULE_DEPS      = 0x00200000,
    RUN_FLAG__TEST                = 0x00400000,
#ifdef SHELL
    RUN_FLAG__SHELL               = 0x00800000,
#endif
#ifdef PIGLET
    RUN_FLAG__PIGLET              = 0x01000000,
#endif
    RUN_FLAG__MEM_CHECK           = 0x02000000,
    RUN_FLAG__TRACK_ON_SYN        = 0x04000000,
    RUN_FLAG__IP_FRAGS_ONLY       = 0x08000000,

    RUN_FLAG__DUMP_RULE_STATE     = 0x10000000,
};

enum OutputFlag
{
    OUTPUT_FLAG__LINE_BUFFER       = 0x00000001,
    OUTPUT_FLAG__VERBOSE_DUMP      = 0x00000002,
    OUTPUT_FLAG__CHAR_DATA         = 0x00000004,
    OUTPUT_FLAG__APP_DATA          = 0x00000008,

    OUTPUT_FLAG__SHOW_DATA_LINK    = 0x00000010,
    OUTPUT_FLAG__USE_UTC           = 0x00000020,
    OUTPUT_FLAG__INCLUDE_YEAR      = 0x00000040,
    /* Note using this alters the packet - can't be used inline */
    OUTPUT_FLAG__OBFUSCATE         = 0x00000080,

    OUTPUT_FLAG__ALERT_IFACE       = 0x00000100,
    OUTPUT_FLAG__NO_TIMESTAMP      = 0x00000200,
    OUTPUT_FLAG__ALERTS            = 0x00000400,
    OUTPUT_FLAG__WIDE_HEX          = 0x00000800,

    OUTPUT_FLAG__ALERT_REFS        = 0x00001000,
};

enum LoggingFlag
{
    LOGGING_FLAG__VERBOSE         = 0x00000001,
    LOGGING_FLAG__QUIET           = 0x00000002,
    LOGGING_FLAG__SYSLOG          = 0x00000004,
    LOGGING_FLAG__SHOW_PLUGINS    = 0x00000008,
};

enum TunnelFlags
{
    TUNNEL_GTP    = 0x01,
    TUNNEL_TEREDO = 0x02,
    TUNNEL_6IN4   = 0x04,
    TUNNEL_4IN6   = 0x08,
    TUNNEL_4IN4   = 0x10,
    TUNNEL_6IN6   = 0x20,
    TUNNEL_GRE    = 0x40,
    TUNNEL_MPLS   = 0x80,
    TUNNEL_VXLAN  = 0x100
};

class FastPatternConfig;
class RuleStateMap;
class TraceConfig;

struct srmm_table_t;
struct sopg_table_t;
struct ClassType;
struct DetectionFilterConfig;
struct EventQueueConfig;
struct FrameworkConfig;
struct HighAvailabilityConfig;
struct IpsActionsConfig;
struct LatencyConfig;
struct MemoryConfig;
struct Plugins;
struct PORT_RULE_MAP;
struct RateFilterConfig;
struct ReferenceSystem;
struct RuleListNode;
struct RulePortTables;
struct SFDAQConfig;
struct SoRules;
struct ThresholdConfig;
struct VarNode;

namespace snort
{
class GHash;
class ProtocolReference;
class ThreadConfig;
class XHash;
struct ProfilerConfig;

class ReloadResourceTuner
{
public:
    static const unsigned RELOAD_MAX_WORK_PER_PACKET = 3;
    // be aggressive when idle as analyzer gets chance once in every second only due to daq timeout
    static const unsigned RELOAD_MAX_WORK_WHEN_IDLE = 32767;

    virtual ~ReloadResourceTuner() = default;

    // returns true if resource tuning required, false otherwise
    virtual bool tinit() = 0;

    // each of these returns true if resource tuning is complete, false otherwise
    virtual bool tune_packet_context() = 0;
    virtual bool tune_idle_context() = 0;

protected:
    ReloadResourceTuner() = default;

    unsigned max_work = RELOAD_MAX_WORK_PER_PACKET;
    unsigned max_work_idle = RELOAD_MAX_WORK_WHEN_IDLE;
};

struct SnortConfig
{
private:
    void init(const SnortConfig* const, ProtocolReference*);

public:
    SnortConfig(const SnortConfig* const other_conf = nullptr);
    SnortConfig(ProtocolReference* protocol_reference);
    ~SnortConfig();

    SnortConfig(const SnortConfig&) = delete;

    void setup();
    void post_setup();
    bool verify() const;

    void merge(SnortConfig*);
    void clone(const SnortConfig* const);

public:
    //------------------------------------------------------
    // non-reloadable stuff (single instance)
    // FIXIT-L non-reloadable stuff should be made static
    static uint32_t warning_flags;

    //------------------------------------------------------
    // alert module stuff
    std::string rule_order;

    SfCidr homenet;

    //------------------------------------------------------
    // output module stuff
#ifdef REG_TEST
    // FIXIT-M builtin modules should set SnortConfig defaults instead
    uint32_t output_flags = OUTPUT_FLAG__WIDE_HEX;
#else
    uint32_t output_flags = 0;
#endif
    uint32_t logging_flags = 0;

    uint32_t tagged_packet_limit = 256;
    uint16_t event_trace_max = 0;

    std::string log_dir;

    //------------------------------------------------------
    // daq stuff
    SFDAQConfig* daq_config;

    //------------------------------------------------------
    // detection module stuff
    // FIXIT-L pcre_match_limit* are interdependent
    // somehow a packet thread needs a much lower setting
    long int pcre_match_limit = 1500;
    long int pcre_match_limit_recursion = 1500;

    int pcre_ovector_size = 0;
    bool pcre_override = true;

    int asn1_mem = 0;
    uint32_t run_flags = 0;

    unsigned offload_limit = 99999;  // disabled
    unsigned offload_threads = 0;    // disabled

#ifdef HAVE_HYPERSCAN
    bool hyperscan_literals = false;
    bool pcre_to_regex = false;
#endif

    bool global_rule_state = false;
    bool global_default_rule_state = true;

    //------------------------------------------------------
    // process stuff

    // user_id and group_id should be initialized to -1 by default, because
    // chown() use this later, -1 means no change to user_id/group_id
    int user_id = -1;
    int group_id = -1;

    bool dirty_pig = false;

    std::string chroot_dir;        /* -t or config chroot */
    std::string include_path;
    std::string plugin_path;
    std::vector<std::string> script_paths;

    mode_t file_mask = 0;

    //------------------------------------------------------
    // decode module stuff
    int mpls_stack_depth = 0;

    uint8_t mpls_payload_type = 0;
    uint8_t num_layers = 0;
    uint8_t max_ip6_extensions = 0;
    uint8_t max_ip_layers = 0;

    bool enable_esp = false;
    bool address_anomaly_check_enabled = false;

    //------------------------------------------------------
    // active stuff
    uint8_t respond_attempts = 0;
    uint8_t max_responses = 0;
    uint8_t min_interval = 0;
    uint8_t* eth_dst = nullptr;

    std::string respond_device;
    std::string output;

    //------------------------------------------------------
    // attribute tables stuff
    std::string attribute_hosts_file;
    uint32_t max_attribute_hosts = 0;
    uint32_t max_attribute_services_per_host = 0;
    uint32_t max_metadata_services = 0;

    //------------------------------------------------------
    // packet module stuff
    bool vlan_agnostic = false;
    bool addressspace_agnostic = false;

    uint64_t pkt_cnt = 0;           /* -n */
    uint64_t pkt_skip = 0;
    uint64_t pkt_pause_cnt = 0;

    std::string bpf_file;          /* -F or config bpf_file */

    //------------------------------------------------------
    // various modules
    FastPatternConfig* fast_pattern_config = nullptr;
    EventQueueConfig* event_queue_config = nullptr;

    /* policy specific? */
    ThresholdConfig* threshold_config = nullptr;
    RateFilterConfig* rate_filter_config = nullptr;
    DetectionFilterConfig* detection_filter_config = nullptr;

    //------------------------------------------------------
    // FIXIT-L command line only stuff, add to conf / module

    uint32_t event_log_id = 0;
    SfCidr obfuscation_net;
    std::string bpf_filter;
    std::string metadata_filter;

    //------------------------------------------------------
    // FIXIT-L non-module stuff - separate config from derived state?
    std::string run_prefix;
    uint16_t id_offset = 0;
    bool id_subdir = false;
    bool id_zero = false;

    bool stdin_rules = false;

    std::string pid_filename;
    std::string orig_log_dir;      /* set in case of chroot */

    int thiszone = 0;

    std::unordered_map<std::string, ClassType*> classifications;
    std::unordered_map<std::string, ReferenceSystem*> references;

    RuleStateMap* rule_states = nullptr;
    GHash* otn_map = nullptr;

    ProtocolReference* proto_ref = nullptr;

    unsigned num_rule_types = 0;
    RuleListNode* rule_lists = nullptr;
    int evalOrder[Actions::MAX + 1];

    IpsActionsConfig* ips_actions_config = nullptr;
    FrameworkConfig* framework_config = nullptr;

    /* master port list table */
    RulePortTables* port_tables = nullptr;

    /* The port-rule-maps map the src-dst ports to rules for
     * udp and tcp, for Ip we map the dst port as the protocol,
     * and for Icmp we map the dst port to the Icmp type. This
     * allows us to use the decode packet information to in O(1)
     * select a group of rules to apply to the packet.  These
     * rules may or may not have content.  We process the content
     * 1st and then the no content rules for udp/tcp and icmp, and
     * then we process the ip rules. */
    PORT_RULE_MAP* prmIpRTNX = nullptr;
    PORT_RULE_MAP* prmIcmpRTNX = nullptr;
    PORT_RULE_MAP* prmTcpRTNX = nullptr;
    PORT_RULE_MAP* prmUdpRTNX = nullptr;

    srmm_table_t* srmmTable = nullptr;   /* srvc rule map master table */
    srmm_table_t* spgmmTable = nullptr;  /* srvc port_group map master table */
    sopg_table_t* sopgTable = nullptr;   /* service-ordinal to port_group table */

    XHash* detection_option_hash_table = nullptr;
    XHash* detection_option_tree_hash_table = nullptr;
    XHash* rtn_hash_table = nullptr;

    PolicyMap* policy_map = nullptr;
    VarNode* var_list = nullptr;
    std::string tweaks;

    DataBus* global_dbus = nullptr;

    uint16_t tunnel_mask = 0;

    // FIXIT-L this is temporary for legacy paf_max required only for HI;
    // it is not appropriate for multiple stream_tcp with different
    // paf_max; the HI splitter should pull from there
    unsigned max_pdu = 16384;

    //------------------------------------------------------
    ProfilerConfig* profiler = nullptr;
    LatencyConfig* latency = nullptr;

    unsigned remote_control_port = 0;
    std::string remote_control_socket;

    MemoryConfig* memory = nullptr;
    //------------------------------------------------------

    std::vector<ScratchAllocator*> scratchers;
    std::vector<void *>* state = nullptr;
    unsigned num_slots = 0;

    ThreadConfig* thread_config;
    HighAvailabilityConfig* ha_config = nullptr;
    TraceConfig* trace_config = nullptr;

    // TraceConfig instance which used by TraceSwap control channel command
    TraceConfig* overlay_trace_config = nullptr;

    //------------------------------------------------------
    //Reload inspector related

    bool cloned = false;
    Plugins* plugins = nullptr;
    SoRules* so_rules = nullptr;
private:
    std::list<ReloadResourceTuner*> reload_tuners;

public:
    //------------------------------------------------------
    // decoding related
    uint8_t get_num_layers() const
    { return num_layers; }

    // curr_layer is the zero based ip6 options
    bool hit_ip6_maxopts(uint8_t curr_opt) const
    { return max_ip6_extensions && (curr_opt >= max_ip6_extensions); }

    // curr_ip is the zero based ip layer
    bool hit_ip_maxlayers(uint8_t curr_ip) const
    { return max_ip_layers && (curr_ip >= max_ip_layers); }

    //------------------------------------------------------
    // Non-static mutator methods

    void add_plugin_path(const char*);
    void add_script_path(const char*);
    void enable_syslog();
    void set_alert_before_pass(bool);
    void set_alert_mode(const char*);
    void set_chroot_dir(const char*);
    void set_create_pid_file(bool);
    void set_daemon(bool);
    void set_decode_data_link(bool);
    void set_dirty_pig(bool);
    void set_dst_mac(const char*);
    void set_dump_chars_only(bool);
    void set_dump_payload(bool);
    void set_dump_payload_verbose(bool);
    void set_gid(const char*);
    void set_log_dir(const char*);
    void set_log_mode(const char*);
    void set_no_logging_timestamps(bool);
    void set_obfuscate(bool);
    void set_obfuscation_mask(const char*);
    void set_include_path(const char*);
    void set_process_all_events(bool);
    void set_quiet(bool);
    void set_show_year(bool);
    void set_tunnel_verdicts(const char*);
    void set_treat_drop_as_alert(bool);
    void set_treat_drop_as_ignore(bool);
    void set_tweaks(const char*);
    void set_uid(const char*);
    void set_umask(uint32_t);
    void set_utc(bool);
    void set_verbose(bool);
    void set_overlay_trace_config(TraceConfig*);

    //------------------------------------------------------
    // accessor methods

    long int get_mpls_stack_depth() const
    { return mpls_stack_depth; }

    long int get_mpls_payload_type() const
    { return mpls_payload_type; }

    bool mpls_overlapping_ip() const
    { return run_flags & RUN_FLAG__MPLS_OVERLAPPING_IP; }

    bool mpls_multicast() const
    { return run_flags & RUN_FLAG__MPLS_MULTICAST; }

    bool esp_decoding() const
    { return enable_esp; }

    bool is_address_anomaly_check_enabled() const
    { return address_anomaly_check_enabled; }

    // mode related
    bool dump_msg_map() const
    { return run_flags & RUN_FLAG__DUMP_MSG_MAP; }

    bool dump_rule_meta() const
    { return run_flags & RUN_FLAG__DUMP_RULE_META; }

    bool dump_rule_state() const
    { return run_flags & RUN_FLAG__DUMP_RULE_STATE; }

    bool dump_rule_deps() const
    { return run_flags & RUN_FLAG__DUMP_RULE_DEPS; }

    bool dump_rule_info() const
    { return dump_msg_map() or dump_rule_meta() or dump_rule_deps() or dump_rule_state(); }

    bool test_mode() const
    { return run_flags & RUN_FLAG__TEST; }

    bool mem_check() const
    { return run_flags & RUN_FLAG__MEM_CHECK; }

    bool daemon_mode() const
    { return run_flags & RUN_FLAG__DAEMON; }

    bool read_mode() const
    { return run_flags & RUN_FLAG__READ; }

    bool inline_mode() const
    { return get_ips_policy()->policy_mode == POLICY_MODE__INLINE; }

    bool inline_test_mode() const
    { return get_ips_policy()->policy_mode == POLICY_MODE__INLINE_TEST; }

    bool show_file_codes() const
    { return run_flags & RUN_FLAG__SHOW_FILE_CODES; }

    bool adaptor_inline_mode() const
    { return run_flags & RUN_FLAG__INLINE; }

    bool adaptor_inline_test_mode() const
    { return run_flags & RUN_FLAG__INLINE_TEST; }

    // logging stuff
    bool log_syslog() const
    { return logging_flags & LOGGING_FLAG__SYSLOG; }

    bool log_verbose() const
    { return logging_flags & LOGGING_FLAG__VERBOSE; }

    bool log_quiet() const
    { return logging_flags & LOGGING_FLAG__QUIET; }

    // event stuff
    uint32_t get_event_log_id() const
    { return event_log_id; }

    bool process_all_events() const
    { return event_queue_config->process_all_events; }

    int get_eval_index(Actions::Type type) const
    { return evalOrder[type]; }

    // output stuff
    bool output_include_year() const
    { return output_flags & OUTPUT_FLAG__INCLUDE_YEAR; }

    bool output_use_utc() const
    { return output_flags & OUTPUT_FLAG__USE_UTC; }

    bool output_datalink() const
    { return output_flags & OUTPUT_FLAG__SHOW_DATA_LINK; }

    bool verbose_byte_dump() const
    { return output_flags & OUTPUT_FLAG__VERBOSE_DUMP; }

    bool obfuscate() const
    { return output_flags & OUTPUT_FLAG__OBFUSCATE; }

    bool output_app_data() const
    { return output_flags & OUTPUT_FLAG__APP_DATA; }

    bool output_char_data() const
    { return output_flags & OUTPUT_FLAG__CHAR_DATA; }

    bool alert_interface() const
    { return output_flags & OUTPUT_FLAG__ALERT_IFACE; }

    bool output_no_timestamp() const
    { return output_flags & OUTPUT_FLAG__NO_TIMESTAMP; }

    bool line_buffered_logging() const
    { return output_flags & OUTPUT_FLAG__LINE_BUFFER; }

    bool output_wide_hex() const
    { return output_flags & OUTPUT_FLAG__WIDE_HEX; }

    bool alert_refs() const
    { return output_flags & OUTPUT_FLAG__ALERT_REFS; }

    // run flags
    bool no_lock_pid_file() const
    { return run_flags & RUN_FLAG__NO_LOCK_PID_FILE; }

    bool create_pid_file() const
    { return run_flags & RUN_FLAG__CREATE_PID_FILE; }

    bool pcap_show() const
    { return run_flags & RUN_FLAG__PCAP_SHOW; }

    bool treat_drop_as_alert() const
    { return run_flags & RUN_FLAG__TREAT_DROP_AS_ALERT; }

    bool treat_drop_as_ignore() const
    { return run_flags & RUN_FLAG__TREAT_DROP_AS_IGNORE; }

    bool alert_before_pass() const
    { return run_flags & RUN_FLAG__ALERT_BEFORE_PASS; }

    bool no_pcre() const
    { return run_flags & RUN_FLAG__NO_PCRE; }

    bool conf_error_out() const
    { return run_flags & RUN_FLAG__CONF_ERROR_OUT; }

    bool assure_established() const
    { return run_flags & RUN_FLAG__ASSURE_EST; }

    // other stuff
    uint8_t min_ttl() const
    { return get_network_policy()->min_ttl; }

    uint8_t new_ttl() const
    { return get_network_policy()->new_ttl; }

    long int get_pcre_match_limit() const
    { return pcre_match_limit; }

    long int get_pcre_match_limit_recursion() const
    { return pcre_match_limit_recursion; }

    const ProfilerConfig* get_profiler() const
    { return profiler; }

    long int get_tagged_packet_limit() const
    { return tagged_packet_limit; }

    uint32_t get_max_attribute_hosts() const
    { return max_attribute_hosts; }

    uint32_t get_max_services_per_host() const
    { return max_attribute_services_per_host; }

    int get_uid() const
    { return user_id; }

    int get_gid() const
    { return group_id; }

    bool get_vlan_agnostic() const
    { return vlan_agnostic; }

    bool address_space_agnostic() const
    { return addressspace_agnostic; }

    bool change_privileges() const
    { return user_id != -1 || group_id != -1 || !chroot_dir.empty(); }

    bool track_on_syn() const
    { return (run_flags & RUN_FLAG__TRACK_ON_SYN) != 0; }

    bool ip_frags_only() const
    { return (run_flags & RUN_FLAG__IP_FRAGS_ONLY) != 0; }

    void clear_run_flags(RunFlag flag)
    { run_flags &= ~flag; }

    void set_run_flags(RunFlag flag)
    { run_flags |= flag; }

    const std::list<ReloadResourceTuner*>& get_reload_resource_tuners() const
    { return reload_tuners; }

    void clear_reload_resource_tuner_list()
    { reload_tuners.clear(); }

    bool get_default_rule_state() const;

    SO_PUBLIC bool tunnel_bypass_enabled(uint16_t proto) const;

    // FIXIT-L snort_conf needed for static hash before initialized
    static bool static_hash()
    { return get_conf() && get_conf()->run_flags & RUN_FLAG__STATIC_HASH; }

    // This requests an entry in the scratch space vector and calls setup /
    // cleanup as appropriate
    SO_PUBLIC static int request_scratch(ScratchAllocator*);
    SO_PUBLIC static void release_scratch(int);

    // runtime access to const config - especially for packet threads
    // prefer access via packet->context->conf
    SO_PUBLIC static const SnortConfig* get_conf();

    // runtime access to mutable config - main thread only, and only special cases
    SO_PUBLIC static SnortConfig* get_main_conf();

    static void set_conf(const SnortConfig*);

    SO_PUBLIC void register_reload_resource_tuner(ReloadResourceTuner& rrt)
    { reload_tuners.push_back(&rrt); }

    static void cleanup_fatal_error();
};
}

#endif
