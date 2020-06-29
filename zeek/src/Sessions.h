// See the file "COPYING" in the main distribution directory for copyright.

#pragma once

#include "Frag.h"
#include "PacketFilter.h"
#include "NetVar.h"
#include "analyzer/protocol/tcp/Stats.h"

#include <map>
#include <utility>

#include <sys/types.h> // for u_char

class EncapsulationStack;
class EncapsulatingConn;
class Packet;
class PacketProfiler;
class Connection;
class ConnCompressor;
struct ConnID;

class Discarder;

namespace analyzer { namespace stepping_stone { class SteppingStoneManager; } }
namespace analyzer { namespace arp { class ARP_Analyzer; } }

struct SessionStats {
	size_t num_TCP_conns;
	size_t max_TCP_conns;
	uint64_t cumulative_TCP_conns;

	size_t num_UDP_conns;
	size_t max_UDP_conns;
	uint64_t cumulative_UDP_conns;

	size_t num_ICMP_conns;
	size_t max_ICMP_conns;
	uint64_t cumulative_ICMP_conns;

	size_t num_fragments;
	size_t max_fragments;
	uint64_t num_packets;
};

class NetSessions {
public:
	NetSessions();
	~NetSessions();

	// Main entry point for packet processing.
	void NextPacket(double t, const Packet* pkt);

	void Done();	// call to drain events before destructing

	// Returns a reassembled packet, or nil if there are still
	// some missing fragments.
	FragReassembler* NextFragment(double t, const IP_Hdr* ip,
				const u_char* pkt);

	// Looks up the connection referred to by the given Val,
	// which should be a conn_id record.  Returns nil if there's
	// no such connection or the Val is ill-formed.
	Connection* FindConnection(Val* v);

	void Remove(Connection* c);
	void Remove(FragReassembler* f);

	void Insert(Connection* c);

	// Generating connection_pending events for all connections
	// that are still active.
	void Drain();

	// Clears the session maps.
	void Clear();

	void GetStats(SessionStats& s) const;

	void Weird(const char* name, const Packet* pkt,
	    const EncapsulationStack* encap = nullptr, const char* addl = "");
	void Weird(const char* name, const IP_Hdr* ip,
	    const EncapsulationStack* encap = nullptr, const char* addl = "");

	PacketFilter* GetPacketFilter()
		{
		if ( ! packet_filter )
			packet_filter = new PacketFilter(packet_filter_default);
		return packet_filter;
		}

	analyzer::stepping_stone::SteppingStoneManager* GetSTPManager()	{ return stp_manager; }

	unsigned int CurrentConnections()
		{
		return tcp_conns.size() + udp_conns.size() + icmp_conns.size();
		}

	void DoNextPacket(double t, const Packet *pkt, const IP_Hdr* ip_hdr,
			const EncapsulationStack* encapsulation);

	/**
	 * Wrapper that recurses on DoNextPacket for encapsulated IP packets.
	 *
	 * @param t Network time.
	 * @param pkt If the outer pcap header is available, this pointer can be set
	 *        so that the fake pcap header passed to DoNextPacket will use
	 *        the same timeval.  The caplen and len fields of the fake pcap
	 *        header are always set to the TotalLength() of \a inner.
	 * @param inner Pointer to IP header wrapper of the inner packet, ownership
	 *        of the pointer's memory is assumed by this function.
	 * @param prev Any previous encapsulation stack of the caller, not including
	 *        the most-recently found depth of encapsulation.
	 * @param ec The most-recently found depth of encapsulation.
	 */
	void DoNextInnerPacket(double t, const Packet *pkt,
	                      const IP_Hdr* inner, const EncapsulationStack* prev,
	                      const EncapsulatingConn& ec);

	/**
	 * Recurses on DoNextPacket for encapsulated Ethernet/IP packets.
	 *
	 * @param t  Network time.
	 * @param pkt  If the outer pcap header is available, this pointer can be
	 *        set so that the fake pcap header passed to DoNextPacket will use
	 *        the same timeval.
	 * @param caplen  number of captured bytes remaining
	 * @param len  number of bytes remaining as claimed by outer framing
	 * @param data  the remaining packet data
	 * @param link_type  layer 2 link type used for initializing inner packet
	 * @param prev  Any previous encapsulation stack of the caller, not
	 *        including the most-recently found depth of encapsulation.
	 * @param ec The most-recently found depth of encapsulation.
	 */
	void DoNextInnerPacket(double t, const Packet* pkt,
                           uint32_t caplen, uint32_t len,
                           const u_char* data, int link_type,
                           const EncapsulationStack* prev,
                           const EncapsulatingConn& ec);

	/**
	 * Returns a wrapper IP_Hdr object if \a pkt appears to be a valid IPv4
	 * or IPv6 header based on whether it's long enough to contain such a header,
	 * if version given in the header matches the proto argument, and also checks
	 * that the payload length field of that header matches the actual
	 * length of \a pkt given by \a caplen.
	 *
	 * @param caplen The length of \a pkt in bytes.
	 * @param pkt The inner IP packet data.
	 * @param proto Either IPPROTO_IPV6 or IPPROTO_IPV4 to indicate which IP
	 *        protocol \a pkt corresponds to.
	 * @param inner The inner IP packet wrapper pointer to be allocated/assigned
	 *        if \a pkt looks like a valid IP packet or at least long enough
	 *        to hold an IP header.
	 * @return 0 If the inner IP packet appeared valid, else -1 if \a caplen
	 *         is greater than the supposed IP packet's payload length field, -2
	 *         if the version of the inner header does not match proto or
	 *         1 if \a caplen is less than the supposed packet's payload length.
	 *         In the -1 case, \a inner may still be non-null if \a caplen was
	 *         long enough to be an IP header, and \a inner is always non-null
	 *         for other return values.
	 */
	int ParseIPPacket(int caplen, const u_char* const pkt, int proto,
	                  IP_Hdr*& inner);

	unsigned int ConnectionMemoryUsage();
	unsigned int ConnectionMemoryUsageConnVals();
	unsigned int MemoryAllocation();
	analyzer::tcp::TCPStateStats tcp_stats;	// keeps statistics on TCP states

protected:
	friend class ConnCompressor;
	friend class IPTunnelTimer;

	using ConnectionMap = std::map<ConnIDKey, Connection*>;
	using FragmentMap = std::map<FragReassemblerKey, FragReassembler*>;

	Connection* NewConn(const ConnIDKey& k, double t, const ConnID* id,
			const u_char* data, int proto, uint32_t flow_label,
			const Packet* pkt, const EncapsulationStack* encapsulation);

	Connection* LookupConn(const ConnectionMap& conns, const ConnIDKey& key);

	// Returns true if the port corresonds to an application
	// for which there's a Bro analyzer (even if it might not
	// be used by the present policy script), or it's more
	// generally a likely server port, false otherwise.
	//
	// Note, port is in host order.
	bool IsLikelyServerPort(uint32_t port,
				TransportProto transport_proto) const;

	// Upon seeing the first packet of a connection, checks whether
	// we want to analyze it (e.g., we may not want to look at partial
	// connections), and, if yes, whether we should flip the roles of
	// originator and responder (based on known ports or such).
	// Use tcp_flags=0 for non-TCP.
	bool WantConnection(uint16_t src_port, uint16_t dest_port,
				TransportProto transport_proto,
				uint8_t tcp_flags, bool& flip_roles);

	// Record the given packet (if a dumper is active).  If len=0
	// then the whole packet is recorded, otherwise just the first
	// len bytes.
	void DumpPacket(const Packet *pkt, int len=0);

	// For a given protocol, checks whether the header's length as derived
	// from lower-level headers or the length actually captured is less
	// than that protocol's minimum header size.
	bool CheckHeaderTrunc(int proto, uint32_t len, uint32_t caplen,
			      const Packet *pkt, const EncapsulationStack* encap);

	// Inserts a new connection into the sessions map. If a connection with
	// the same key already exists in the map, it will be overwritten by
	// the new one.  Connection count stats get updated either way (so most
	// cases should likely check that the key is not already in the map to
	// avoid unnecessary incrementing of connecting counts).
	void InsertConnection(ConnectionMap* m, const ConnIDKey& key, Connection* conn);

	ConnectionMap tcp_conns;
	ConnectionMap udp_conns;
	ConnectionMap icmp_conns;
	FragmentMap fragments;

	SessionStats stats;

	using IPPair = std::pair<IPAddr, IPAddr>;
	using TunnelActivity = std::pair<EncapsulatingConn, double>;
	using IPTunnelMap = std::map<IPPair, TunnelActivity>;
	IPTunnelMap ip_tunnels;

	analyzer::arp::ARP_Analyzer* arp_analyzer;

	analyzer::stepping_stone::SteppingStoneManager* stp_manager;
	Discarder* discarder;
	PacketFilter* packet_filter;
	uint64_t num_packets_processed;
	PacketProfiler* pkt_profiler;
	bool dump_this_packet;	// if true, current packet should be recorded
};


class IPTunnelTimer final : public Timer {
public:
	IPTunnelTimer(double t, NetSessions::IPPair p)
	: Timer(t + zeek::BifConst::Tunnel::ip_tunnel_timeout,
			TIMER_IP_TUNNEL_INACTIVITY), tunnel_idx(p) {}

	~IPTunnelTimer() override {}

	void Dispatch(double t, bool is_expire) override;

protected:
	NetSessions::IPPair tunnel_idx;
};


class FragReassemblerTracker {
public:
	FragReassemblerTracker(NetSessions* s, FragReassembler* f)
		: net_sessions(s), frag_reassembler(f)
		{ }

	~FragReassemblerTracker()
		{ net_sessions->Remove(frag_reassembler); }

private:
	NetSessions* net_sessions;
	FragReassembler* frag_reassembler;
};

// Manager for the currently active sessions.
extern NetSessions* sessions;
