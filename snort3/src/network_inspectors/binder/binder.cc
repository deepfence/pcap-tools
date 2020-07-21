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
// binder.cc author Russ Combs <rucombs@cisco.com>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <iomanip>
#include <sstream>

#include "detection/detection_engine.h"
#include "flow/flow.h"
#include "flow/flow_key.h"
#include "framework/data_bus.h"
#include "log/messages.h"
#include "main/snort_config.h"
#include "managers/inspector_manager.h"
#include "managers/module_manager.h"
#include "profiler/profiler.h"
#include "protocols/packet.h"
#include "protocols/tcp.h"
#include "protocols/udp.h"
#include "pub_sub/assistant_gadget_event.h"
#include "stream/stream.h"
#include "stream/stream_splitter.h"
#include "target_based/host_attributes.h"
#include "target_based/snort_protocols.h"

#include "bind_module.h"
#include "binding.h"

using namespace snort;
using namespace std;

THREAD_LOCAL ProfileStats bindPerfStats;

// FIXIT-P these lookups should be optimized when the dust settles
#define INS_IP   "stream_ip"
#define INS_ICMP "stream_icmp"
#define INS_TCP  "stream_tcp"
#define INS_UDP  "stream_udp"
#define INS_USER "stream_user"
#define INS_FILE "stream_file"

//-------------------------------------------------------------------------
// binding
//-------------------------------------------------------------------------

Binding::Binding()
{
    when.split_nets = false;
    when.src_nets = nullptr;
    when.dst_nets = nullptr;

    when.split_ports = false;
    when.src_ports.set();
    when.dst_ports.set();

    when.split_zones = false;
    when.src_zones.set();
    when.dst_zones.set();

    when.protos = PROTO_BIT__ANY_TYPE;
    when.vlans.set();
    when.ifaces.reset();

    when.ips_id = 0;
    when.ips_id_user = 0;
    when.role = BindWhen::BR_EITHER;

    use.inspection_index = 0;
    use.ips_index = 0;
    use.action = BindUse::BA_INSPECT;

    use.what = BindUse::BW_NONE;
    use.object = nullptr;
}

Binding::~Binding()
{
    if ( when.src_nets )
        sfvar_free(when.src_nets);

    if ( when.dst_nets )
        sfvar_free(when.dst_nets);
}

inline bool Binding::check_ips_policy(const Flow* flow) const
{
    if ( !when.ips_id )
        return true;

    if ( when.ips_id == flow->ips_policy_id )
        return true;

    return false;
}

inline bool Binding::check_addr(const Flow* flow) const
{
    if ( when.split_nets )
        return true;

    if ( !when.src_nets )
        return true;

    switch ( when.role )
    {
        case BindWhen::BR_SERVER:
            if ( sfvar_ip_in(when.src_nets, &flow->server_ip) )
                return true;
            break;

        case BindWhen::BR_CLIENT:
            if ( sfvar_ip_in(when.src_nets, &flow->client_ip) )
                return true;
            break;

        case BindWhen::BR_EITHER:
            if ( sfvar_ip_in(when.src_nets, &flow->client_ip) or
                   sfvar_ip_in(when.src_nets, &flow->server_ip) )
                return true;
            break;

        default:
            break;
    }
    return false;
}

inline bool Binding::check_proto(const Flow* flow) const
{
    if ( when.protos & BIT((unsigned)flow->pkt_type) )
        return true;

    return false;
}

inline bool Binding::check_iface(const Packet* p) const
{
    if ( !p or when.ifaces.none() )
        return true;

    auto in = p->pkth->ingress_index;
    auto out = p->pkth->egress_index;

    if ( in > 0 and when.ifaces.test(out) )
        return true;

    if ( out > 0 and when.ifaces.test(in) )
        return true;

    return false;
}

inline bool Binding::check_vlan(const Flow* flow) const
{
    unsigned v = flow->key->vlan_tag;
    return when.vlans.test(v);
}

inline bool Binding::check_port(const Flow* flow) const
{
    if ( when.split_ports )
        return true;

    switch ( when.role )
    {
        case BindWhen::BR_SERVER:
            return when.src_ports.test(flow->server_port);
        case BindWhen::BR_CLIENT:
            return when.src_ports.test(flow->client_port);
        case BindWhen::BR_EITHER:
            return (when.src_ports.test(flow->client_port) or
                when.src_ports.test(flow->server_port) );
        default:
            break;
    }
    return false;
}

inline bool Binding::check_service(const Flow* flow) const
{
    if ( !flow->service )
        return when.svc.empty();

    if ( when.svc == flow->service )
        return true;

    return false;
}

inline bool Binding::check_service(const char* service) const
{
    if ( when.svc == service )
        return true;

    return false;
}

// we want to correlate src_zone to src_nets and src_ports, and dst_zone to dst_nets and
// dst_ports. it doesn't matter if the packet is actually moving in the opposite direction as
// binder is only evaluated once per flow and we need to capture the correct binding from
// either side of the conversation
template<typename When, typename Traffic, typename Compare>
static Binding::DirResult directional_match(const When& when_src, const When& when_dst,
    const Traffic& traffic_src, const Traffic& traffic_dst,
    const Binding::DirResult dr, const Compare& compare)
{
    bool src_in_src = false;
    bool src_in_dst = false;
    bool dst_in_src = false;
    bool dst_in_dst = false;
    bool forward_match = false;
    bool reverse_match = false;

    switch ( dr )
    {
        case Binding::DR_ANY_MATCH:
            src_in_src = compare(when_src, traffic_src);
            src_in_dst = compare(when_dst, traffic_src);
            dst_in_src = compare(when_src, traffic_dst);
            dst_in_dst = compare(when_dst, traffic_dst);

            forward_match = src_in_src and dst_in_dst;
            reverse_match = dst_in_src and src_in_dst;

            if ( forward_match and reverse_match )
                return dr;

            if ( forward_match )
                return Binding::DR_FORWARD;

            if ( reverse_match )
                return Binding::DR_REVERSE;

            return Binding::DR_NO_MATCH;

        case Binding::DR_FORWARD:
            src_in_src = compare(when_src, traffic_src);
            dst_in_dst = compare(when_dst, traffic_dst);
            return src_in_src and dst_in_dst ? dr : Binding::DR_NO_MATCH;

        case Binding::DR_REVERSE:
            src_in_dst = compare(when_dst, traffic_src);
            dst_in_src = compare(when_src, traffic_dst);
            return src_in_dst and dst_in_src ? dr : Binding::DR_NO_MATCH;

        default:
            break;
    }

    return Binding::DR_NO_MATCH;
}

inline Binding::DirResult Binding::check_split_addr(
    const Flow* flow, const Packet* p, const Binding::DirResult dr) const
{
    if ( !when.split_nets )
        return dr;

    if ( !when.src_nets && !when.dst_nets )
        return dr;

    const SfIp* src_ip;
    const SfIp* dst_ip;

    if ( p && p->ptrs.ip_api.is_ip() )
    {
        src_ip = p->ptrs.ip_api.get_src();
        dst_ip = p->ptrs.ip_api.get_dst();
    }
    else
    {
        src_ip = &flow->client_ip;
        dst_ip = &flow->server_ip;
    }

    return directional_match(when.src_nets, when.dst_nets, src_ip, dst_ip, dr,
        [](sfip_var_t* when_val, const SfIp* traffic_val)
        { return when_val ? sfvar_ip_in(when_val, traffic_val) : true; });
}

inline Binding::DirResult Binding::check_split_port(
    const Flow* flow, const Packet* p, const Binding::DirResult dr) const
{
    if ( !when.split_ports )
        return dr;

    uint16_t src_port;
    uint16_t dst_port;

    if ( !p )
    {
        src_port = flow->client_port;
        dst_port = flow->server_port;
    }
    else if ( p->is_tcp() or p->is_udp() )
    {
        src_port = p->ptrs.sp;
        dst_port = p->ptrs.dp;
    }
    else
        return dr;

    return directional_match(when.src_ports, when.dst_ports, src_port, dst_port, dr,
        [](const PortBitSet& when_val, uint16_t traffic_val)
        { return when_val.test(traffic_val); });
}

inline bool Binding::check_zone(const Packet* p) const
{
    if ( when.split_zones or !p )
        return true;

    if (p->pkth->egress_group == DAQ_PKTHDR_UNKNOWN or
        p->pkth->ingress_group == DAQ_PKTHDR_UNKNOWN)
        return true;

    assert(((unsigned)p->pkth->ingress_group) < when.src_zones.size());
    assert(((unsigned)p->pkth->egress_group) < when.dst_zones.size());

    if (when.src_zones.test((unsigned)p->pkth->ingress_group) or
        when.dst_zones.test((unsigned)p->pkth->egress_group))
        return true;
    return false;
}

inline Binding::DirResult Binding::check_split_zone(const Packet* p, const Binding::DirResult dr) const
{
    if ( !when.split_zones )
        return dr;

    int src_zone;
    int dst_zone;

    if ( p )
    {
        src_zone = p->pkth->ingress_group;
        dst_zone = p->pkth->egress_group;
    }
    else
        return dr;

    return directional_match(when.src_zones, when.dst_zones, src_zone, dst_zone, dr,
        [](const ZoneBitSet& when_val, int traffic_val)
        { return traffic_val == DAQ_PKTHDR_UNKNOWN ? true : when_val.test(traffic_val); });
}

bool Binding::check_all(const Flow* flow, Packet* p, const char* service) const
{
    Binding::DirResult dir = Binding::DR_ANY_MATCH;

    if ( !check_ips_policy(flow) )
        return false;

    if ( !check_iface(p) )
        return false;

    if ( !check_vlan(flow) )
        return false;

    // FIXIT-M need to check role and addr/ports relative to it
    if ( !check_addr(flow) )
        return false;

    dir = check_split_addr(flow, p, dir);
    if ( dir == Binding::DR_NO_MATCH )
        return false;

    if ( !check_proto(flow) )
        return false;

    if ( !check_port(flow) )
        return false;

    dir = check_split_port(flow, p, dir);
    if ( dir == Binding::DR_NO_MATCH )
        return false;

    dir = check_split_zone(p, dir);
    if ( dir == Binding::DR_NO_MATCH )
        return false;

    if (service)
    {
        if (!check_service(service))
            return false;
    }
    else if ( !check_service(flow) )
        return false;

    if ( !check_zone(p) )
        return false;

    return true;
}

//-------------------------------------------------------------------------
// helpers
//-------------------------------------------------------------------------

static void set_session(Flow* flow, const char* key)
{
    Inspector* pin = InspectorManager::get_inspector(key);

    if ( pin )
    {
        // FIXIT-M need to set ssn client and server independently
        flow->set_client(pin);
        flow->set_server(pin);
        flow->clouseau = nullptr;
    }
}

static void set_session(Flow* flow)
{
    flow->ssn_client = nullptr;
    flow->ssn_server = nullptr;
    flow->clouseau = nullptr;
}

static void set_service(Flow* flow, const HostAttributeEntry* host)
{
    Stream::set_snort_protocol_id(flow, host, FROM_SERVER);
}

static Inspector* get_gadget(Flow* flow)
{
    if ( flow->ssn_state.snort_protocol_id == UNKNOWN_PROTOCOL_ID )
        return nullptr;

    const char* s = SnortConfig::get_conf()->proto_ref->get_name(flow->ssn_state.snort_protocol_id);

    return InspectorManager::get_inspector_by_service(s);
}

static Inspector* get_gadget_by_id(const char* service)
{
    const SnortConfig* sc = SnortConfig::get_conf();
    const SnortProtocolId id = sc->proto_ref->find(service);
    const char* s = sc->proto_ref->get_name(id);
    return InspectorManager::get_inspector_by_service(s);
}

static std::string to_string(const sfip_var_t* list)
{
    std::string ipset;

    if ( !list or !list->head )
        return "";

    for (auto node = list->head; node; node = node->next)
    {
        SfIpString ip_str;
        auto ip = node->ip;

        ip->get_addr()->ntop(ip_str);
        ipset += std::string(ip_str);

        if ( ((ip->get_family() == AF_INET6) and (ip->get_bits() != 128)) or
            ((ip->get_family() == AF_INET ) and (ip->get_bits() != 32 )) )
        {
            auto bits = ip->get_bits();
            bits -= (ip->get_family() == AF_INET and bits) ? 96 : 0;
            ipset += "/" + std::to_string(bits);
        }

        ipset += ", ";
    }

    if ( !ipset.empty() )
        ipset.erase(ipset.end() - 2, ipset.end());

    return ipset;
}

template <unsigned N>
static std::string to_string(const std::bitset<N>& bitset)
{
    std::stringstream ss;

    if ( bitset.none() or bitset.all() )
        return "";

    for (unsigned i = 0; i < bitset.size(); ++i)
        if ( bitset[i] )
            ss << i << " ";

    auto str = ss.str();
    if ( !str.empty() )
        str.pop_back();

    return str;
}

static std::string to_string(const BindWhen::Role& role)
{
    switch( role )
    {
    case BindWhen::BR_CLIENT:
        return "client";
    case BindWhen::BR_SERVER:
        return "server";
    default:
        return "";
    }
}

static std::string proto_to_string(unsigned proto)
{
    switch( proto )
    {
    case PROTO_BIT__IP:
        return "ip";
    case PROTO_BIT__ICMP:
        return "icmp";
    case PROTO_BIT__TCP:
        return "tcp";
    case PROTO_BIT__UDP:
        return "udp";
    case PROTO_BIT__PDU:
        return "user";
    case PROTO_BIT__FILE:
        return "file";
    default:
        return "";
    }
}

static std::string to_string(const BindUse::Action& action)
{
    switch( action )
    {
    case BindUse::BA_RESET:
        return "reset";
    case BindUse::BA_BLOCK:
        return "block";
    case BindUse::BA_ALLOW:
        return "allow";
    default:
        return "";
    }
}

static std::string to_string(const BindWhen& bw)
{
    std::string when;

    when += "{";

    if ( bw.ips_id_user )
        when += " ips_policy_id = " + std::to_string(bw.ips_id_user) + ",";

    if ( !bw.svc.empty() )
        when += " service = " + bw.svc + ",";

    auto role = to_string(bw.role);
    if ( !role.empty() )
        when += " role = " + role + ",";

    auto proto = proto_to_string(bw.protos);
    if ( !proto.empty() )
        when += " proto = " + proto + ",";

    auto src_nets = to_string(bw.src_nets);
    auto dst_nets = to_string(bw.dst_nets);

    if ( !src_nets.empty() )
        when += (bw.split_nets ? " src_nets = " : " nets = ") + src_nets + ",";
    if ( bw.split_nets and !dst_nets.empty() )
        when += " dst_nets = " + dst_nets + ",";

    auto src_ports = to_string<65536>(bw.src_ports);
    auto dst_ports = to_string<65536>(bw.dst_ports);

    if ( !src_ports.empty() )
        when += (bw.split_ports ? " src_ports = " : " ports = ") + src_ports + ",";
    if ( bw.split_ports and !dst_ports.empty() )
        when += " dst_ports = " + dst_ports + ",";

    auto src_zones = to_string<64>(bw.src_zones);
    auto dst_zones = to_string<64>(bw.dst_zones);

    if ( !src_zones.empty() )
        when += (bw.split_zones ? " src_zones = " : " zones = ") + src_zones + ",";
    if ( bw.split_zones and !dst_zones.empty() )
        when += " dst_zones = " + dst_zones + ",";

    auto ifaces = to_string<256>(bw.ifaces);
    if ( !ifaces.empty() )
        when += " ifaces = " + ifaces + ",";

    auto vlans = to_string<4096>(bw.vlans);
    if ( !vlans.empty() )
        when += " vlans = " + vlans + ",";

    if ( when.length() > 1 )
        when.pop_back();

    when += " }";

    return when;
}

static std::string to_string(const BindUse& bu)
{
    std::string use;

    use += "{";

    auto action = to_string(bu.action);
    if ( !action.empty() )
        use += " action = " + action + ",";

    if ( !bu.svc.empty() )
        use += " service = " + bu.svc + ",";

    if ( !bu.type.empty() )
        use += " type = " + ((bu.type.at(0) == '.') ? bu.type.substr(1) : bu.type) + ",";

    if ( !bu.name.empty() and (bu.type != bu.name) )
        use += " name = " + bu.name + ",";

    if ( use.length() > 1 )
        use.pop_back();

    use += " }";

    return use;
}

//-------------------------------------------------------------------------
// stuff stuff
//-------------------------------------------------------------------------

struct Stuff
{
    BindUse::Action action;

    Inspector* client;
    Inspector* server;
    Inspector* wizard;
    Inspector* gadget;
    Inspector* data;

    Stuff()
    {
        action = BindUse::BA_INSPECT;
        client = server = nullptr;
        wizard = gadget = nullptr;
        data = nullptr;
    }

    bool update(Binding*);

    bool apply_action(Flow*);
    void apply_session(Flow*, const HostAttributeEntry*);
    void apply_service(Flow*, const HostAttributeEntry*);
    void apply_assistant(Flow*, const char*);
};

bool Stuff::update(Binding* pb)
{
    if ( pb->use.action != BindUse::BA_INSPECT )
    {
        action = pb->use.action;
        return true;
    }

    switch ( pb->use.what )
    {
    case BindUse::BW_NONE:
        break;
    case BindUse::BW_PASSIVE:
        data = (Inspector*)pb->use.object;
        break;
    case BindUse::BW_CLIENT:
        client = (Inspector*)pb->use.object;
        break;
    case BindUse::BW_SERVER:
        server = (Inspector*)pb->use.object;
        break;
    case BindUse::BW_STREAM:
        client = server = (Inspector*)pb->use.object;
        break;
    case BindUse::BW_WIZARD:
        wizard = (Inspector*)pb->use.object;
        return true;
    case BindUse::BW_GADGET:
        gadget = (Inspector*)pb->use.object;
        return true;
    default:
        break;
    }
    return false;
}

bool Stuff::apply_action(Flow* flow)
{
    switch ( action )
    {
    case BindUse::BA_RESET:
        flow->set_state(Flow::FlowState::RESET);
        return false;

    case BindUse::BA_BLOCK:
        flow->set_state(Flow::FlowState::BLOCK);
        return false;

    case BindUse::BA_ALLOW:
        flow->set_state(Flow::FlowState::ALLOW);
        return false;

    default:
        break;
    }
    flow->set_state(Flow::FlowState::INSPECT);
    return true;
}

void Stuff::apply_session(Flow* flow, const HostAttributeEntry* host)
{
    if ( server )
    {
        flow->set_server(server);

        if ( client )
            flow->set_client(client);
        else
            flow->set_client(server);

        return;
    }

    switch ( flow->pkt_type )
    {
    case PktType::IP:
        set_session(flow, INS_IP);
        flow->ssn_policy = host ? host->hostInfo.fragPolicy : 0;
        break;

    case PktType::ICMP:
        set_session(flow, INS_ICMP);
        break;

    case PktType::TCP:
        set_session(flow, INS_TCP);
        flow->ssn_policy = host ? host->hostInfo.streamPolicy : 0;
        break;

    case PktType::UDP:
        set_session(flow, INS_UDP);
        break;

    case PktType::PDU:
        set_session(flow, INS_USER);
        break;

    case PktType::FILE:
        set_session(flow, INS_FILE);
        break;

    default:
        set_session(flow);
    }
}

void Stuff::apply_service(Flow* flow, const HostAttributeEntry* host)
{
    if ( data )
        flow->set_data(data);

    if ( host )
        set_service(flow, host);

    if ( !gadget )
        gadget = get_gadget(flow);

    if ( gadget )
    {
        flow->set_gadget(gadget);

        if ( flow->ssn_state.snort_protocol_id == UNKNOWN_PROTOCOL_ID )
            flow->ssn_state.snort_protocol_id = gadget->get_service();

        DataBus::publish(SERVICE_INSPECTOR_CHANGE_EVENT, DetectionEngine::get_current_packet());
    }

    else if ( wizard )
        flow->set_clouseau(wizard);
}

void Stuff::apply_assistant(Flow* flow, const char* service)
{
    if ( !gadget )
        gadget = get_gadget_by_id(service);

    if ( gadget )
        flow->set_assistant_gadget(gadget);
}

//-------------------------------------------------------------------------
// class stuff
//-------------------------------------------------------------------------

class Binder : public Inspector
{
public:
    Binder(vector<Binding*>&);
    ~Binder() override;

    void remove_inspector_binding(SnortConfig*, const char*) override;

    bool configure(SnortConfig*) override;
    void show(const SnortConfig*) const override;

    void eval(Packet*) override { }

    void add(Binding* b)
    { bindings.emplace_back(b); }

    void handle_flow_setup(Packet*);
    void handle_flow_service_change(Flow*);
    void handle_new_standby_flow(Flow*);
    void handle_assistant_gadget(const char* service, Packet*);

private:
    void set_binding(SnortConfig*, Binding*);
    void get_bindings(Flow*, Stuff&, Packet* = nullptr, const char* = nullptr); // may be null when dealing with HA flows
    void apply(Flow*, Stuff&);
    void apply_assistant(Flow*, Stuff&, const char*);
    Inspector* find_gadget(Flow*);

private:
    vector<Binding*> bindings;
};

class FlowStateSetupHandler : public DataHandler
{
public:
    FlowStateSetupHandler() : DataHandler(BIND_NAME) { }

    void handle(DataEvent& event, Flow* flow) override
    {
        Binder* binder = InspectorManager::get_binder();
        if (binder && flow)
            binder->handle_flow_setup(const_cast<Packet*>(event.get_packet()));
    }
};

// When a flow's service changes, re-evaluate service to inspector mapping.
class FlowServiceChangeHandler : public DataHandler
{
public:
    FlowServiceChangeHandler() : DataHandler(BIND_NAME) { }

    void handle(DataEvent&, Flow* flow) override
    {
        Binder* binder = InspectorManager::get_binder();
        if (binder && flow)
            binder->handle_flow_service_change(flow);
    }
};

class StreamHANewFlowHandler : public DataHandler
{
public:
    StreamHANewFlowHandler() : DataHandler(BIND_NAME) { }

    void handle(DataEvent&, Flow* flow) override
    {
        Binder* binder = InspectorManager::get_binder();
        if (binder && flow)
            binder->handle_new_standby_flow(flow);
    }
};

class AssistantGadgetHandler : public DataHandler
{
public:
    AssistantGadgetHandler() : DataHandler(BIND_NAME) { }

    void handle(DataEvent& event, Flow*) override
    {
        Binder* binder = InspectorManager::get_binder();
        AssistantGadgetEvent* assistant_event = (AssistantGadgetEvent*)&event;

        if (binder)
            binder->handle_assistant_gadget(assistant_event->get_service(),
                assistant_event->get_packet());
    }
};

Binder::Binder(vector<Binding*>& v)
{
    bindings = std::move(v);
}

Binder::~Binder()
{
    for ( auto* p : bindings )
        delete p;
}

bool Binder::configure(SnortConfig* sc)
{
    unsigned sz = bindings.size();

    for ( unsigned i = 0; i < sz; i++ )
    {
        Binding* pb = bindings[i];

        // Update with actual policy indices instead of user provided names
        if ( pb->when.ips_id_user )
        {
            IpsPolicy* p = sc->policy_map->get_user_ips(pb->when.ips_id_user);
            if ( p )
                pb->when.ips_id = p->policy_id;
            else
                ParseError("can't bind. ips_policy_id %u does not exist", pb->when.ips_id);
        }

        if ( !pb->use.ips_index and !pb->use.inspection_index )
            set_binding(sc, pb);
    }

    DataBus::subscribe(FLOW_STATE_SETUP_EVENT, new FlowStateSetupHandler());
    DataBus::subscribe(FLOW_SERVICE_CHANGE_EVENT, new FlowServiceChangeHandler());
    DataBus::subscribe(STREAM_HA_NEW_FLOW_EVENT, new StreamHANewFlowHandler());
    DataBus::subscribe(FLOW_ASSISTANT_GADGET_EVENT, new AssistantGadgetHandler());

    return true;
}

void Binder::show(const SnortConfig*) const
{
    std::once_flag once;

    if ( !bindings.size() )
        return;

    for (const auto& b : bindings)
    {
        std::call_once(once, []{ ConfigLogger::log_option("bindings"); });

        auto bind_when = "{ when = " + to_string(b->when) + ",";
        auto bind_use = "use = " + to_string(b->use) + " }";
        ConfigLogger::log_list("", bind_when.c_str(), "   ");
        ConfigLogger::log_list("", bind_use.c_str(), "   ", true);
    }
}

void Binder::remove_inspector_binding(SnortConfig*, const char* name)
{
    vector<Binding*>::iterator it;
    for ( it = bindings.begin(); it != bindings.end(); ++it )
    {
        const char* key;
        Binding *pb = *it;
        if ( pb->use.svc.empty() )
            key = pb->use.name.c_str();
        else
            key = pb->use.svc.c_str();
        if ( !strcmp(key, name) )
        {
            bindings.erase(it);
            delete pb;
            return;
        }
    }
}

void Binder::handle_flow_setup(Packet* p)
{
    Profile profile(bindPerfStats);
    Stuff stuff;
    Flow* flow = p->flow;

    get_bindings(flow, stuff, p);
    apply(flow, stuff);

    ++bstats.verdicts[stuff.action];
    ++bstats.packets;
}

void Binder::handle_flow_service_change( Flow* flow )
{
    Profile profile(bindPerfStats);

    assert(flow);

    Inspector* ins = nullptr;
    if (flow->service)
    {
        ins = find_gadget(flow);
        if ( flow->gadget != ins )
        {
            if ( flow->gadget )
                flow->clear_gadget();
            if ( ins )
            {
                flow->set_gadget(ins);
                flow->ssn_state.snort_protocol_id = ins->get_service();
                DataBus::publish(SERVICE_INSPECTOR_CHANGE_EVENT, DetectionEngine::get_current_packet());
            }
            else
                flow->ssn_state.snort_protocol_id = UNKNOWN_PROTOCOL_ID;
        }
    }
    else
    {
        // reset to wizard when service is not specified
        unsigned sz = bindings.size();
        for ( unsigned i = 0; i < sz; i++ )
        {
            Binding* pb = bindings[i];
            if ( pb->use.ips_index or pb->use.inspection_index )
                continue;

            if ( pb->use.what == BindUse::BW_WIZARD )
            {
                ins = (Inspector*)pb->use.object;
                break;
            }
        }

        if ( flow->gadget )
            flow->clear_gadget();
        if ( flow->clouseau )
            flow->clear_clouseau();
        if ( ins )
        {
            flow->set_clouseau(ins);
        }
        flow->ssn_state.snort_protocol_id = UNKNOWN_PROTOCOL_ID;
    }

    // If there is no inspector bound to this flow after the service change, see if there's at least
    // an associated protocol ID.
    if ( !ins && flow->service )
        flow->ssn_state.snort_protocol_id = SnortConfig::get_conf()->proto_ref->find(flow->service);

    if ( !flow->is_stream() )
        return;

    if ( ins )
    {
        Stream::set_splitter(flow, true, ins->get_splitter(true));
        Stream::set_splitter(flow, false, ins->get_splitter(false));
    }
    else
    {
        Stream::set_splitter(flow, true, new AtomSplitter(true));
        Stream::set_splitter(flow, false, new AtomSplitter(false));
    }
}

void Binder::handle_new_standby_flow( Flow* flow )
{
    Profile profile(bindPerfStats);

    Stuff stuff;
    get_bindings(flow, stuff);
    apply(flow, stuff);

    ++bstats.verdicts[stuff.action];
}

void Binder::handle_assistant_gadget(const char* service, Packet* p)
{
    Profile profile(bindPerfStats);
    Stuff stuff;
    Flow* flow = p->flow;

    get_bindings(flow, stuff, p, service);
    apply_assistant(flow, stuff, service);
}

//-------------------------------------------------------------------------
// implementation stuff
//-------------------------------------------------------------------------

void Binder::set_binding(SnortConfig* sc, Binding* pb)
{
    if ( pb->use.action != BindUse::BA_INSPECT )
        return;

    const char* key = pb->use.name.c_str();
    Module* m = ModuleManager::get_module(key);
    bool is_global = m ? m->get_usage() == Module::GLOBAL : false;

    pb->use.object = InspectorManager::get_inspector(key, is_global, sc);

    if ( pb->use.object )
    {
        switch ( ((Inspector*)pb->use.object)->get_api()->type )
        {
        case IT_STREAM: pb->use.what = BindUse::BW_STREAM; break;
        case IT_WIZARD: pb->use.what = BindUse::BW_WIZARD; break;
        case IT_SERVICE: pb->use.what = BindUse::BW_GADGET; break;
        case IT_PASSIVE: pb->use.what = BindUse::BW_PASSIVE; break;
        default: break;
        }
    }
    if ( !pb->use.object )
        pb->use.what = BindUse::BW_NONE;

    if ( pb->use.what == BindUse::BW_NONE )
        ParseError("can't bind %s", key);
}

// FIXIT-P this is a simple linear search until functionality is nailed
// down.  performance should be the focus of the next iteration.
void Binder::get_bindings(Flow* flow, Stuff& stuff, Packet* p, const char* service)
{
    unsigned sz = bindings.size();

    // Evaluate policy ID bindings first
    // FIXIT-P The way these are being used, the policy bindings should be a separate list if not a
    //          separate table entirely
    // FIXIT-L This will select the first policy ID of each type that it finds and ignore the rest.
    //          It gets potentially hairy if people start specifying overlapping policy types in
    //          overlapping rules.
    bool inspection_set = false, ips_set = false;
    const SnortConfig* sc = SnortConfig::get_conf();

    for ( unsigned i = 0; i < sz; i++ )
    {
        Binding* pb = bindings[i];

        // Skip any rules that don't contain an ID for a policy type we haven't set yet.
        if ( (!pb->use.inspection_index or inspection_set) and
             (!pb->use.ips_index or ips_set) )
            continue;

        if ( !pb->check_all(flow, p, service) )
            continue;

        if ( pb->use.inspection_index and !inspection_set )
        {
            set_inspection_policy(sc, pb->use.inspection_index - 1);
            if (!service)
                flow->inspection_policy_id = pb->use.inspection_index - 1;
            inspection_set = true;
        }

        if ( pb->use.ips_index and !ips_set )
        {
            set_ips_policy(sc, pb->use.ips_index - 1);
            if (!service)
                flow->ips_policy_id = pb->use.ips_index - 1;
            ips_set = true;
        }

    }

    Binder* sub = InspectorManager::get_binder();

    // If policy selection produced a new binder to use, use that instead.
    if ( sub && sub != this )
    {
        sub->get_bindings(flow, stuff, p);
        return;
    }

    // If we got here, that means that a sub-policy with a binder was not invoked.
    // Continue using this binder for the rest of processing.
    for ( unsigned i = 0; i < sz; i++ )
    {
        Binding* pb = bindings[i];

        if ( pb->use.ips_index or pb->use.inspection_index )
            continue;

        if ( !pb->check_all(flow, p, service) )
            continue;

        if ( stuff.update(pb) )
            return;
    }
}

Inspector* Binder::find_gadget(Flow* flow)
{
    Stuff stuff;
    get_bindings(flow, stuff);
    return stuff.gadget;
}

void Binder::apply(Flow* flow, Stuff& stuff)
{
    // setup action
    if ( !stuff.apply_action(flow) )
        return;

    const HostAttributeEntry* host = HostAttributes::find_host(&flow->server_ip);

    // setup session
    stuff.apply_session(flow, host);

    // setup service
    stuff.apply_service(flow, host);
}

void Binder::apply_assistant(Flow* flow, Stuff& stuff, const char* service)
{
    stuff.apply_assistant(flow, service);
}

//-------------------------------------------------------------------------
// api stuff
//-------------------------------------------------------------------------

static Module* mod_ctor()
{ return new BinderModule; }

static void mod_dtor(Module* m)
{ delete m; }

static Inspector* bind_ctor(Module* m)
{
    BinderModule* mod = (BinderModule*)m;
    vector<Binding*>& pb = mod->get_data();
    return new Binder(pb);
}

static void bind_dtor(Inspector* p)
{
    delete p;
}

static const InspectApi bind_api =
{
    {
        PT_INSPECTOR,
        sizeof(InspectApi),
        INSAPI_VERSION,
        0,
        API_RESERVED,
        API_OPTIONS,
        BIND_NAME,
        BIND_HELP,
        mod_ctor,
        mod_dtor
    },
    IT_PASSIVE,
    PROTO_BIT__ANY_TYPE,
    nullptr, // buffers
    nullptr, // service
    nullptr, // pinit
    nullptr, // pterm
    nullptr, // tinit
    nullptr, // tterm
    bind_ctor,
    bind_dtor,
    nullptr, // ssn
    nullptr  // reset
};

const BaseApi* nin_binder = &bind_api.base;

