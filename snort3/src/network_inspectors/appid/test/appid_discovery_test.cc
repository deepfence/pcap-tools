//--------------------------------------------------------------------------
// Copyright (C) 2018-2020 Cisco and/or its affiliates. All rights reserved.
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
// appid_discovery_test.cc author Masud Hasan <mashasan@cisco.com>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define APPID_MOCK_INSPECTOR_H // avoiding mocked inspector

#include "host_tracker/host_cache.h"
#include "network_inspectors/appid/appid_discovery.cc"
#include "network_inspectors/appid/appid_peg_counts.h"

#include "search_engines/search_tool.h"
#include "utils/sflsq.cc"

#include "appid_mock_session.h"
#include "appid_session_api.h"
#include "tp_lib_handler.h"

#include <CppUTest/CommandLineTestRunner.h>
#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>

namespace snort
{
// Stubs for packet
Packet::Packet(bool) {}
Packet::~Packet() {}
bool Packet::get_ip_proto_next(unsigned char&, IpProtocol&) const { return true; }

// Stubs for inspector
Inspector::Inspector()
{
    set_api(nullptr);
}
Inspector::~Inspector() = default;
bool Inspector::likes(Packet*) { return true; }
bool Inspector::get_buf(const char*, Packet*, InspectionBuffer&) { return true; }
class StreamSplitter* Inspector::get_splitter(bool) { return nullptr; }

// Stubs for module
Module::Module(char const*, char const*) {}
void Module::sum_stats(bool) {}
void Module::show_interval_stats(std::vector<unsigned>&, FILE*) {}
void Module::show_stats() {}
void Module::reset_stats() {}
PegCount Module::get_global_count(char const*) const { return 0; }

// Stubs for logs
void LogMessage(const char*,...) {}
void ErrorMessage(const char*,...) {}
void LogLabel(const char*, FILE*) {}

// Stubs for utils
char* snort_strdup(const char* str)
{
    assert(str);
    size_t n = strlen(str) + 1;
    char* p = (char*)snort_alloc(n);
    memcpy(p, str, n);
    return p;
}
char* snort_strndup(const char* src, size_t)
{
    return snort_strdup(src);
}
time_t packet_time() { return std::time(nullptr); }

// Stubs for search_tool
SearchTool::SearchTool(const char*, bool) {}
SearchTool::~SearchTool() {}
void SearchTool::add(const char*, unsigned, int, bool) {}
void SearchTool::add(const char*, unsigned, void*, bool) {}
void SearchTool::add(const uint8_t*, unsigned, int, bool) {}
void SearchTool::add(const uint8_t*, unsigned, void*, bool) {}

// Stubs for ip
namespace ip
{
void IpApi::set(const SfIp& sip, const SfIp& dip)
{
    type = IAT_DATA;
    src = sip;
    dst = dip;
    iph = nullptr;
}
} // namespace ip

} // namespace snort

// Stubs for publish
void DataBus::publish(const char*, DataEvent& event, Flow*)
{
    AppidEvent* appid_event = (AppidEvent*)&event;
    char* test_log = (char*)mock().getData("test_log").getObjectPointer();
    snprintf(test_log, 256, "Published change_bits == %s",
        appid_event->get_change_bitset().to_string().c_str());
    mock().actualCall("publish");
}

// Stubs for matchers
static HttpPatternMatchers* http_matchers;
DnsPatternMatchers::~DnsPatternMatchers() { }
HttpPatternMatchers::~HttpPatternMatchers() {}
void HttpPatternMatchers::get_http_offsets(Packet*, AppIdHttpSession*) {}
SipPatternMatchers::~SipPatternMatchers() { }
SslPatternMatchers::~SslPatternMatchers() { }

void ApplicationDescriptor::set_id(const Packet&, AppIdSession&, AppidSessionDirection, AppId, AppidChangeBits&) { }
void ApplicationDescriptor::set_id(AppId app_id){my_id = app_id;}
void ServiceAppDescriptor::set_id(AppId app_id, OdpContext& odp_ctxt)
{
    set_id(app_id);
    deferred = odp_ctxt.get_app_info_mgr().get_app_info_flags(app_id, APPINFO_FLAG_DEFER);
}
void ServiceAppDescriptor::update_stats(AppId){}
void ServiceAppDescriptor::set_port_service_id(AppId){}
void ClientAppDescriptor::update_user(AppId, const char*){}
void ClientAppDescriptor::update_stats(AppId) {}
void PayloadAppDescriptor::update_stats(AppId) {}

// Stubs for AppIdModule
AppIdModule::AppIdModule(): Module("appid_mock", "appid_mock_help") {}
AppIdModule::~AppIdModule() {}
void AppIdModule::sum_stats(bool) {}
void AppIdModule::show_dynamic_stats() {}
bool AppIdModule::begin(char const*, int, SnortConfig*) { return true; }
bool AppIdModule::end(char const*, int, SnortConfig*) { return true; }
bool AppIdModule::set(char const*, Value&, SnortConfig*) { return true; }
const Command* AppIdModule::get_commands() const { return nullptr; }
const PegInfo* AppIdModule::get_pegs() const { return nullptr; }
PegCount* AppIdModule::get_counts() const { return nullptr; }
ProfileStats* AppIdModule::get_profile() const { return nullptr; }
void AppIdModule::set_trace(const Trace*) const { }
const TraceOption* AppIdModule::get_trace_options() const { return nullptr; }
THREAD_LOCAL bool ThirdPartyAppIdContext::tp_reload_in_progress = false;

// Stubs for config
static AppIdConfig app_config;
static AppIdContext app_ctxt(app_config);
AppId OdpContext::get_port_service_id(IpProtocol, uint16_t)
{
    return APP_ID_NONE;
}

AppId OdpContext::get_protocol_service_id(IpProtocol)
{
    return APP_ID_NONE;
}

// Stubs for AppIdInspector
AppIdInspector::AppIdInspector(AppIdModule&) { ctxt = &stub_ctxt; }
AppIdInspector::~AppIdInspector() = default;
void AppIdInspector::eval(Packet*) { }
bool AppIdInspector::configure(SnortConfig*) { return true; }
void AppIdInspector::show(const SnortConfig*) const { }
void AppIdInspector::tinit() { }
void AppIdInspector::tterm() { }
AppIdContext& AppIdInspector::get_ctxt() const
{
    assert(ctxt);
    return *ctxt;
}

// Stubs for AppInfoManager
AppInfoTableEntry* AppInfoManager::get_app_info_entry(AppId)
{
    return nullptr;
}
const char* AppInfoManager::get_app_name(int32_t)
{
    return nullptr;
}

// Stubs for AppIdSession
void AppIdSession::sync_with_snort_protocol_id(AppId, Packet*) {}
void AppIdSession::check_app_detection_restart(AppidChangeBits&) {}
void AppIdSession::set_client_appid_data(AppId, AppidChangeBits&, char*) {}
void AppIdSession::examine_rtmp_metadata(AppidChangeBits&) {}
void AppIdSession::examine_ssl_metadata(AppidChangeBits&) {}
void AppIdSession::update_encrypted_app_id(AppId) {}
bool AppIdSession::is_tp_processing_done() const {return 0;}
AppIdSession* AppIdSession::allocate_session(const Packet*, IpProtocol,
    AppidSessionDirection, AppIdInspector*)
{
    return nullptr;
}

void AppIdSession::publish_appid_event(AppidChangeBits& change_bits, Flow* flow, bool, uint32_t)
{
    static AppIdSessionApi api(*this);
    AppidEvent app_event(change_bits, false, 0, api);
    DataBus::publish(APPID_EVENT_ANY_CHANGE, app_event, flow);
}

void AppIdHttpSession::set_tun_dest(){}

// Stubs for ServiceDiscovery
void ServiceDiscovery::initialize() {}
void ServiceDiscovery::finalize_service_patterns() {}
void ServiceDiscovery::match_by_pattern(AppIdSession&, const Packet*, IpProtocol) {}
void ServiceDiscovery::get_port_based_services(IpProtocol, uint16_t, AppIdSession&) {}
void ServiceDiscovery::get_next_service(const Packet*, const AppidSessionDirection, AppIdSession&) {}
int ServiceDiscovery::identify_service(AppIdSession&, Packet*, AppidSessionDirection,
    AppidChangeBits&) { return 0; }
int ServiceDiscovery::add_ftp_service_state(AppIdSession&) { return 0; }
bool ServiceDiscovery::do_service_discovery(AppIdSession&, Packet*, AppidSessionDirection,
    AppidChangeBits&) { return 0; }
int ServiceDiscovery::incompatible_data(AppIdSession&, const Packet*,AppidSessionDirection,
    ServiceDetector*) { return 0; }
int ServiceDiscovery::fail_service(AppIdSession&, const Packet*, AppidSessionDirection,
    ServiceDetector*, ServiceDiscoveryState*) { return 0; }
int ServiceDiscovery::add_service_port(AppIdDetector*,
    const ServiceDetectorPort&) { return APPID_EINVALID; }
static AppIdModule* s_app_module = nullptr;
static AppIdInspector* s_ins = nullptr;
static ServiceDiscovery* s_discovery_manager = nullptr;

HostCacheIp host_cache(50);
AppId HostTracker::get_appid(Port, IpProtocol, bool, bool)
{
    return APP_ID_NONE;
}

// Stubs for ClientDiscovery
void ClientDiscovery::initialize() {}
void ClientDiscovery::finalize_client_plugins() {}
static ClientDiscovery* c_discovery_manager = new ClientDiscovery();
bool ClientDiscovery::do_client_discovery(AppIdSession&, Packet*,
    AppidSessionDirection, AppidChangeBits&)
{
    return false;
}

// Stubs for misc items
HostPortVal* HostPortCache::find(const SfIp*, uint16_t, IpProtocol, const OdpContext&)
{
    return nullptr;
}
void AppIdServiceState::check_reset(AppIdSession&, const SfIp*, uint16_t) {}
int dns_host_scan_hostname(const uint8_t*, size_t, AppId*, AppId*)
{
    return 0;
}
bool do_tp_discovery(ThirdPartyAppIdContext& , AppIdSession&, IpProtocol,
    Packet*, AppidSessionDirection&, AppidChangeBits&)
{
    return true;
}
TPLibHandler* TPLibHandler::self = nullptr;
THREAD_LOCAL AppIdStats appid_stats;
THREAD_LOCAL AppIdDebug* appidDebug = nullptr;
void AppIdDebug::activate(const Flow*, const AppIdSession*, bool) { active = false; }
AppId find_length_app_cache(const LengthKey&)
{
    return APP_ID_NONE;
}
void check_session_for_AF_indicator(Packet*, AppidSessionDirection, AppId, const OdpContext&) {}
AppId check_session_for_AF_forecast(AppIdSession&, Packet*, AppidSessionDirection, AppId)
{
    return APP_ID_UNKNOWN;
}

bool AppIdReloadTuner::tinit() { return false; }

bool AppIdReloadTuner::tune_resources(unsigned int)
{
    return true;
}

TEST_GROUP(appid_discovery_tests)
{
    char test_log[256];
    void setup() override
    {
        appidDebug = new AppIdDebug();
        http_matchers = new HttpPatternMatchers;
        s_app_module = new AppIdModule;
        s_ins = new AppIdInspector(*s_app_module);
        AppIdPegCounts::init_pegs();
        mock().setDataObject("test_log", "char", test_log);
    }

    void teardown() override
    {
        delete appidDebug;
        delete http_matchers;
        if (s_discovery_manager)
        {
            delete s_discovery_manager;
            s_discovery_manager = nullptr;
        }
        if (c_discovery_manager)
        {
            delete c_discovery_manager;
            c_discovery_manager = nullptr;
        }
        delete s_ins;
        delete s_app_module;
        AppIdPegCounts::cleanup_pegs();
        AppIdPegCounts::cleanup_peg_info();
        mock().clear();
    }
};

TEST(appid_discovery_tests, event_published_when_ignoring_flow)
{
    // Testing event from do_pre_discovery() path
    mock().expectOneCall("publish");
    test_log[0] = '\0';
    Packet p;
    p.packet_flags = 0;
    DAQ_PktHdr_t pkth;
    p.pkth = &pkth;
    SfIp ip;
    p.ptrs.ip_api.set(ip, ip);
    AppIdModule app_module;
    AppIdInspector ins(app_module);
    AppIdSession* asd = new AppIdSession(IpProtocol::TCP, nullptr, 21, ins);
    Flow* flow = new Flow;
    flow->set_flow_data(asd);
    p.flow = flow;
    asd->initiator_port = 21;
    asd->initiator_ip.set("1.2.3.4");
    asd->set_session_flags(APPID_SESSION_FUTURE_FLOW);

    AppIdDiscovery::do_application_discovery(&p, ins, nullptr);

    // Detect changes in service, client, payload, and misc appid
    mock().checkExpectations();
    STRCMP_EQUAL(test_log, "Published change_bits == 0000000011110");
    delete asd;
    delete flow;
}

TEST(appid_discovery_tests, event_published_when_processing_flow)
{
    // Testing event from do_discovery() path
    mock().expectOneCall("publish");
    test_log[0] = '\0';
    Packet p;
    p.packet_flags = 0;
    DAQ_PktHdr_t pkth;
    p.pkth = &pkth;
    SfIp ip;
    p.ptrs.ip_api.set(ip, ip);
    p.ptrs.tcph = nullptr;
    AppIdModule app_module;
    AppIdInspector ins(app_module);
    AppIdSession* asd = new AppIdSession(IpProtocol::TCP, nullptr, 21, ins);
    Flow* flow = new Flow;
    flow->set_flow_data(asd);
    p.flow = flow;
    asd->initiator_port = 21;
    asd->initiator_ip.set("1.2.3.4");

    AppIdDiscovery::do_application_discovery(&p, ins, nullptr);

    // Detect changes in service, client, payload, and misc appid
    mock().checkExpectations();
    STRCMP_EQUAL(test_log, "Published change_bits == 0000000011110");
    delete asd;
    delete flow;
}

TEST(appid_discovery_tests, change_bits_for_client_version)
{
    // Testing set_version
    AppidChangeBits change_bits;
    AppIdModule app_module;
    AppIdInspector ins(app_module);
    AppIdSession* asd = new AppIdSession(IpProtocol::TCP, nullptr, 21, ins);
    const char* version = "3.0";
    asd->client.set_version(version, change_bits);

    // Detect changes in client version
    CHECK_EQUAL(change_bits.test(APPID_VERSION_BIT), true);
    delete asd;
}

TEST(appid_discovery_tests, change_bits_for_tls_host)
{
    // Testing set_tls_host
    AppidChangeBits change_bits;
    char* host = snort_strdup(APPID_UT_TLS_HOST);
    TlsSession tls;
    tls.set_tls_host(host, 0, change_bits);

    // Detect changes in tls_host
    CHECK_EQUAL(change_bits.test(APPID_TLSHOST_BIT), true);
}

TEST(appid_discovery_tests, change_bits_for_non_http_appid)
{
    // Testing FTP appid
    mock().expectNCalls(2, "publish");
    Packet p;
    p.packet_flags = 0;
    DAQ_PktHdr_t pkth;
    p.pkth = &pkth;
    SfIp ip;
    p.ptrs.ip_api.set(ip, ip);
    AppIdModule app_module;
    AppIdInspector ins(app_module);
    AppIdSession* asd = new AppIdSession(IpProtocol::TCP, nullptr, 21, ins);
    Flow* flow = new Flow;
    flow->set_flow_data(asd);
    p.flow = flow;
    p.ptrs.tcph = nullptr;
    asd->initiator_port = 21;
    asd->initiator_ip.set("1.2.3.4");
    asd->misc_app_id = APP_ID_NONE;
    asd->payload.set_id(APP_ID_NONE);
    asd->client.set_id(APP_ID_CURL);
    asd->service.set_id(APP_ID_FTP, app_ctxt.get_odp_ctxt());

    AppIdDiscovery::do_application_discovery(&p, ins, nullptr);

    // Detect event for FTP service and CURL client
    CHECK_EQUAL(asd->client.get_id(), APP_ID_CURL);
    CHECK_EQUAL(asd->service.get_id(), APP_ID_FTP);

    // Testing DNS appid
    asd->misc_app_id = APP_ID_NONE;
    asd->payload.set_id(APP_ID_NONE);
    asd->client.set_id(APP_ID_NONE);
    asd->service.set_id(APP_ID_DNS, app_ctxt.get_odp_ctxt());
    AppIdDiscovery::do_application_discovery(&p, ins, nullptr);

    // Detect event for DNS service
    mock().checkExpectations();
    CHECK_EQUAL(asd->service.get_id(), APP_ID_DNS);

    delete asd;
    delete flow;
}

TEST(appid_discovery_tests, change_bits_to_string)
{
    // Testing that all bits from AppidChangeBit enum get translated
    AppidChangeBits change_bits;
    std::string str;

    // Detect empty
    change_bits_to_string(change_bits, str);
    STRCMP_EQUAL(str.c_str(), "");

    // Detect all; failure of this test means some bits from enum are missed in translation
    change_bits.set();
    change_bits_to_string(change_bits, str);
    STRCMP_EQUAL(str.c_str(), "created, service, client, payload, misc, referred, host,"
        " tls-host, url, user-agent, response, referrer, version");

    // Failure of this test is a reminder that enum is changed, hence translator needs update
    CHECK_EQUAL(APPID_MAX_BIT, 13);
}

int main(int argc, char** argv)
{
    int rc = CommandLineTestRunner::RunAllTests(argc, argv);
    return rc;
}
