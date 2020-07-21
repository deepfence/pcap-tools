//--------------------------------------------------------------------------
// Copyright (C) 2014-2020 Cisco and/or its affiliates. All rights reserved.
// Copyright (C) 2004-2013 Sourcefire, Inc.
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

// reputation_inspect.cc author Hui Cao <huica@cisco.com>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "reputation_inspect.h"

#include "detection/detect.h"
#include "detection/detection_engine.h"
#include "events/event_queue.h"
#include "log/messages.h"
#include "network_inspectors/packet_tracer/packet_tracer.h"
#include "packet_io/active.h"
#include "profiler/profiler.h"
#include "protocols/packet.h"


#include "reputation_parse.h"

using namespace snort;

THREAD_LOCAL ProfileStats reputation_perf_stats;
THREAD_LOCAL ReputationStats reputationstats;

const PegInfo reputation_peg_names[] =
{
{ CountType::SUM, "packets", "total packets processed" },
{ CountType::SUM, "blacklisted", "number of packets blacklisted" },
{ CountType::SUM, "whitelisted", "number of packets whitelisted" },
{ CountType::SUM, "monitored", "number of packets monitored" },
{ CountType::SUM, "memory_allocated", "total memory allocated" },

{ CountType::END, nullptr, nullptr }
};

const char* NestedIPKeyword[] =
{
    "inner",
    "outer",
    "all",
    nullptr
};

const char* WhiteActionOption[] =
{
    "unblack",
    "trust",
    nullptr
};

/*
 * Function prototype(s)
 */
static void snort_reputation(ReputationConfig* GlobalConf, Packet* p);

static inline IPrepInfo* reputation_lookup(ReputationConfig* config, const SfIp* ip)
{
    IPrepInfo* result;

    if (!config->scanlocal)
    {
        if (ip->is_private() )
        {
            return nullptr;
        }
    }

    result = (IPrepInfo*)sfrt_flat_dir8x_lookup(ip, config->ip_list);

    return (result);
}

static inline IPdecision get_reputation(ReputationConfig* config, IPrepInfo* rep_info,
    uint32_t* listid, uint32_t ingress_zone, uint32_t egress_zone)
{
    IPdecision decision = DECISION_NULL;

    /*Walk through the IPrepInfo lists*/
    uint8_t* base = (uint8_t*)config->ip_list;
    ListFiles& list_info =  config->list_files;

    while (rep_info)
    {
        int i;
        for (i = 0; i < NUM_INDEX_PER_ENTRY; i++)
        {
            int list_index = rep_info->list_indexes[i];
            if (!list_index)
                break;
            list_index--;
            if (list_info[list_index]->all_zones_enabled ||
                list_info[list_index]->zones.count(ingress_zone) ||
                list_info[list_index]->zones.count(egress_zone))
            {
                if (WHITELISTED_UNBLACK == (IPdecision)list_info[list_index]->list_type)
                    return DECISION_NULL;
                if (config->priority == (IPdecision)list_info[list_index]->list_type )
                {
                    *listid = list_info[list_index]->list_id;
                    return  ((IPdecision)list_info[list_index]->list_type);
                }
                else if ( decision < list_info[list_index]->list_type)
                {
                    decision = (IPdecision)list_info[list_index]->list_type;
                    *listid = list_info[list_index]->list_id;
                }
            }
        }

        if (!rep_info->next)
            break;
        rep_info = (IPrepInfo*)(&base[rep_info->next]);
    }

    return decision;
}

static bool decision_per_layer(ReputationConfig* config, Packet* p,
    uint32_t ingressZone, uint32_t egressZone, const ip::IpApi& ip_api, IPdecision* decision_final)
{
    const SfIp* ip;
    IPdecision decision;
    IPrepInfo* result;

    ip = ip_api.get_src();
    result = reputation_lookup(config, ip);
    if (result)
    {
        decision = get_reputation(config, result, &p->iplist_id, ingressZone, egressZone);

        if (decision == BLACKLISTED)
            *decision_final = BLACKLISTED_SRC;
        else if (decision == MONITORED)
            *decision_final = MONITORED_SRC;
        else if (decision == WHITELISTED_TRUST)
            *decision_final = WHITELISTED_TRUST_SRC;
        else
            *decision_final = decision;

        if ( config->priority == decision)
            return true;
    }

    ip = ip_api.get_dst();
    result = reputation_lookup(config, ip);
    if (result)
    {
        decision = get_reputation(config, result, &p->iplist_id, ingressZone, egressZone);

        if (decision == BLACKLISTED)
            *decision_final = BLACKLISTED_DST;
        else if (decision == MONITORED)
            *decision_final = MONITORED_DST;
        else if (decision == WHITELISTED_TRUST)
            *decision_final = WHITELISTED_TRUST_DST;
        else
            *decision_final = decision;

        if ( config->priority == decision)
            return true;
    }

    return false;
}

static IPdecision reputation_decision(ReputationConfig* config, Packet* p)
{
    IPdecision decision_final = DECISION_NULL;
    uint32_t ingress_zone = 0;
    uint32_t egress_zone = 0;

    if (p->pkth)
    {
        ingress_zone = p->pkth->ingress_group;
        if (p->pkth->egress_index < 0)
            egress_zone = ingress_zone;
        else
            egress_zone = p->pkth->egress_group;
    }

    if (config->nested_ip == INNER)
    {
        decision_per_layer(config, p, ingress_zone, egress_zone, p->ptrs.ip_api, &decision_final);
        return decision_final;
    }

    // For OUTER or ALL, save current layers, iterate, then restore layers as needed
    ip::IpApi blocked_api;
    ip::IpApi tmp_api = p->ptrs.ip_api;
    int8_t num_layer = 0;
    IpProtocol tmp_next = p->get_ip_proto_next();

    if (config->nested_ip == OUTER)
    {
        layer::set_outer_ip_api(p, p->ptrs.ip_api, p->ip_proto_next, num_layer);
        decision_per_layer(config, p, ingress_zone, egress_zone, p->ptrs.ip_api, &decision_final);
    }
    else if (config->nested_ip == ALL)
    {
        bool done = false;
        IPdecision decision_current = DECISION_NULL;

        while (!done and layer::set_outer_ip_api(p, p->ptrs.ip_api, p->ip_proto_next, num_layer))
        {
            done = decision_per_layer(config, p, ingress_zone, egress_zone, p->ptrs.ip_api,
                &decision_current);
            if (decision_current != DECISION_NULL)
            {
                if (decision_current == BLACKLISTED_SRC or decision_current == BLACKLISTED_DST)
                    blocked_api = p->ptrs.ip_api;
                decision_final = decision_current;
                decision_current = DECISION_NULL;
            }
        }
    }
    else
        assert(false); // Should never hit this

    if (decision_final != BLACKLISTED_SRC and decision_final != BLACKLISTED_DST)
        p->ptrs.ip_api = tmp_api;
    else if (config->nested_ip == ALL and p->ptrs.ip_api != blocked_api)
        p->ptrs.ip_api = blocked_api;

    p->ip_proto_next = tmp_next;
    return decision_final;
}

static void snort_reputation(ReputationConfig* config, Packet* p)
{
    IPdecision decision;

    if (!config->ip_list)
        return;

    decision = reputation_decision(config, p);
    Active* act = p->active;

    if (DECISION_NULL == decision)
        return;

    else if (BLACKLISTED_SRC == decision or BLACKLISTED_DST == decision)
    {
        unsigned blacklist_event = (BLACKLISTED_SRC == decision) ?
            REPUTATION_EVENT_BLACKLIST_SRC : REPUTATION_EVENT_BLACKLIST_DST;

        if (p->flow)
        {
            p->flow->flags.reputation_blacklist = true;
            p->flow->flags.reputation_src_dest = (BLACKLISTED_SRC == decision);
        }

        DetectionEngine::queue_event(GID_REPUTATION, blacklist_event);
        act->drop_packet(p, true);

        // disable all preproc analysis and detection for this packet
        DetectionEngine::disable_all(p);
        act->block_session(p, true);
        act->set_drop_reason("reputation");
        reputationstats.blacklisted++;
        if (PacketTracer::is_active())
            PacketTracer::log("Reputation: packet blacklisted, drop\n");
    }

    else if (MONITORED_SRC == decision or MONITORED_DST == decision)
    {
        unsigned monitor_event = (MONITORED_SRC == decision) ?
            REPUTATION_EVENT_MONITOR_SRC : REPUTATION_EVENT_MONITOR_DST;

        if (p->flow)
        {
            p->flow->flags.reputation_monitor = true;
            p->flow->flags.reputation_src_dest = (MONITORED_SRC == decision);
        }

        DetectionEngine::queue_event(GID_REPUTATION, monitor_event);
        reputationstats.monitored++;
    }

    else if (WHITELISTED_TRUST_SRC == decision or WHITELISTED_TRUST_DST == decision)
    {
        unsigned whitelist_event = (WHITELISTED_TRUST_SRC == decision) ?
            REPUTATION_EVENT_WHITELIST_SRC : REPUTATION_EVENT_WHITELIST_DST;

        if (p->flow)
        {
            p->flow->flags.reputation_whitelist = true;
            p->flow->flags.reputation_src_dest = (WHITELISTED_TRUST_SRC == decision);
        }

        DetectionEngine::queue_event(GID_REPUTATION, whitelist_event);
        act->trust_session(p, true);
        reputationstats.whitelisted++;
    }
}

static unsigned create_reputation_id()
{
    static unsigned reputation_id_tracker = 0;
    if (++reputation_id_tracker == 0)
        ++reputation_id_tracker;
    return reputation_id_tracker;
}

static const char* to_string(NestedIP nip)
{
    switch (nip)
    {
    case INNER:
        return "inner";
    case OUTER:
        return "outer";
    case ALL:
        return "all";
    }

    return "";
}

static const char* to_string(WhiteAction wa)
{
    switch (wa)
    {
    case UNBLACK:
        return "unblack";
    case TRUST:
        return "trust";
    }

    return "";
}

static const char* to_string(IPdecision ipd)
{
    switch (ipd)
    {
    case BLACKLISTED:
        return "blacklisted";
    case WHITELISTED_TRUST:
        return "whitelisted_trust";
    case MONITORED:
        return "monitored";
    case BLACKLISTED_SRC:
        return "blacklisted_src";
    case BLACKLISTED_DST:
        return "blacklisted_dst";
    case WHITELISTED_TRUST_SRC:
        return "whitelisted_trust_src";
    case WHITELISTED_TRUST_DST:
        return "whitelisted_trust_dst";
    case WHITELISTED_UNBLACK:
        return "whitelisted_unblack";
    case MONITORED_SRC:
        return "monitored_src";
    case MONITORED_DST:
        return "monitored_dst";
    case DECISION_NULL:
    case DECISION_MAX:
    default:
        return "";
    }
}

//-------------------------------------------------------------------------
// class stuff
//-------------------------------------------------------------------------

Reputation::Reputation(ReputationConfig* pc)
{
    reputation_id = create_reputation_id();
    config = *pc;
    ReputationConfig* conf = &config;
    if (!config.list_dir.empty())
        read_manifest(MANIFEST_FILENAME, conf);

    add_black_white_List(conf);
    estimate_num_entries(conf);
    if (conf->num_entries <= 0)
    {
        ParseWarning(WARN_CONF,
            "reputation: can't find any whitelist/blacklist entries; disabled.");
        return;
    }

    ip_list_init(conf->num_entries + 1, conf);
    reputationstats.memory_allocated = sfrt_flat_usage(conf->ip_list);
}

void Reputation::show(const SnortConfig*) const
{
    ConfigLogger::log_value("blacklist", config.blacklist_path.c_str());
    ConfigLogger::log_value("list_dir", config.list_dir.c_str());
    ConfigLogger::log_value("memcap", config.memcap);
    ConfigLogger::log_value("nested_ip", to_string(config.nested_ip));
    ConfigLogger::log_value("priority", to_string(config.priority));
    ConfigLogger::log_flag("scan_local", config.scanlocal);
    ConfigLogger::log_value("white (action)", to_string(config.white_action));
    ConfigLogger::log_value("whitelist", config.whitelist_path.c_str());
}

void Reputation::eval(Packet* p)
{
    Profile profile(reputation_perf_stats);

    // precondition - what we registered for
    assert(p->has_ip());

    if (p->is_rebuilt())
        return;

    if (p->flow)
    {
        if (p->flow->reputation_id == reputation_id) // reputation previously checked
            return;
        else
            p->flow->reputation_id = reputation_id; // disable future reputation checking
    }

    snort_reputation(&config, p);
    ++reputationstats.packets;
}

//-------------------------------------------------------------------------
// api stuff
//-------------------------------------------------------------------------

static Module* mod_ctor()
{ return new ReputationModule; }

static void mod_dtor(Module* m)
{ delete m; }


static Inspector* reputation_ctor(Module* m)
{
    ReputationModule* mod = (ReputationModule*)m;
    ReputationConfig* conf = mod->get_data();
    return conf ? new Reputation(conf) : nullptr;
}

static void reputation_dtor(Inspector* p)
{
    delete p;
}

const InspectApi reputation_api =
{
    {
        PT_INSPECTOR,
        sizeof(InspectApi),
        INSAPI_VERSION,
        0,
        API_RESERVED,
        API_OPTIONS,
        REPUTATION_NAME,
        REPUTATION_HELP,
        mod_ctor,
        mod_dtor
    },
    IT_NETWORK,
    PROTO_BIT__ANY_IP,
    nullptr, // buffers
    nullptr, // service
    nullptr, // pinit
    nullptr, // pterm
    nullptr, // tinit
    nullptr, // tterm
    reputation_ctor,
    reputation_dtor,
    nullptr, // ssn
    nullptr  // reset
};

#ifdef BUILDING_SO
SO_PUBLIC const BaseApi* snort_plugins[] =
{
    &reputation_api.base,
    nullptr
};
#else
const BaseApi* nin_reputation = &reputation_api.base;
#endif
