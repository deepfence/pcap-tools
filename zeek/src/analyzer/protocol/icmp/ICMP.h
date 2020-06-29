// See the file "COPYING" in the main distribution directory for copyright.

#pragma once

#include "RuleMatcher.h"
#include "analyzer/Analyzer.h"
#include "net_util.h"

class VectorVal;

namespace analyzer { namespace icmp {

typedef enum {
	ICMP_INACTIVE,	// no packet seen
	ICMP_ACTIVE,	// packets seen
} ICMP_EndpointState;

// We do not have an PIA for ICMP (yet) and therefore derive from
// RuleMatcherState to perform our own matching.
class ICMP_Analyzer final : public analyzer::TransportLayerAnalyzer {
public:
	explicit ICMP_Analyzer(Connection* conn);

	void UpdateConnVal(RecordVal *conn_val) override;

	static analyzer::Analyzer* Instantiate(Connection* conn)
		{ return new ICMP_Analyzer(conn); }

protected:
	void Done() override;
	void DeliverPacket(int len, const u_char* data, bool orig,
					uint64_t seq, const IP_Hdr* ip, int caplen) override;
	bool IsReuse(double t, const u_char* pkt) override;
	unsigned int MemoryAllocation() const override;

	void ICMP_Sent(const struct icmp* icmpp, int len, int caplen, int icmpv6,
	               const u_char* data, const IP_Hdr* ip_hdr);

	void Echo(double t, const struct icmp* icmpp, int len,
			 int caplen, const u_char*& data, const IP_Hdr* ip_hdr);
	void Redirect(double t, const struct icmp* icmpp, int len,
			 int caplen, const u_char*& data, const IP_Hdr* ip_hdr);
	void RouterAdvert(double t, const struct icmp* icmpp, int len,
			 int caplen, const u_char*& data, const IP_Hdr* ip_hdr);
	void NeighborAdvert(double t, const struct icmp* icmpp, int len,
			 int caplen, const u_char*& data, const IP_Hdr* ip_hdr);
	void NeighborSolicit(double t, const struct icmp* icmpp, int len,
			 int caplen, const u_char*& data, const IP_Hdr* ip_hdr);
	void RouterSolicit(double t, const struct icmp* icmpp, int len,
			 int caplen, const u_char*& data, const IP_Hdr* ip_hdr);

	void Describe(ODesc* d) const;

	IntrusivePtr<RecordVal> BuildICMPVal(const struct icmp* icmpp, int len,
	                                     int icmpv6, const IP_Hdr* ip_hdr);

	void NextICMP4(double t, const struct icmp* icmpp, int len, int caplen,
			const u_char*& data, const IP_Hdr* ip_hdr );

	IntrusivePtr<RecordVal> ExtractICMP4Context(int len, const u_char*& data);

	void Context4(double t, const struct icmp* icmpp, int len, int caplen,
			const u_char*& data, const IP_Hdr* ip_hdr);

	TransportProto GetContextProtocol(const IP_Hdr* ip_hdr, uint32_t* src_port,
			uint32_t* dst_port);

	void NextICMP6(double t, const struct icmp* icmpp, int len, int caplen,
			const u_char*& data, const IP_Hdr* ip_hdr );

	IntrusivePtr<RecordVal> ExtractICMP6Context(int len, const u_char*& data);

	void Context6(double t, const struct icmp* icmpp, int len, int caplen,
			const u_char*& data, const IP_Hdr* ip_hdr);

	// RFC 4861 Neighbor Discover message options
	IntrusivePtr<VectorVal> BuildNDOptionsVal(int caplen, const u_char* data);

	IntrusivePtr<RecordVal> icmp_conn_val;
	int type;
	int code;
	int request_len, reply_len;

	RuleMatcherState matcher_state;

private:
	void UpdateEndpointVal(const IntrusivePtr<Val>& endp, bool is_orig);
};

// Returns the counterpart type to the given type (e.g., the counterpart
// to ICMP_ECHOREPLY is ICMP_ECHO).
extern int ICMP4_counterpart(int icmp_type, int icmp_code, bool& is_one_way);
extern int ICMP6_counterpart(int icmp_type, int icmp_code, bool& is_one_way);

} } // namespace analyzer::*
