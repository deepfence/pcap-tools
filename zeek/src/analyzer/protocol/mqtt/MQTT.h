// Generated by binpac_quickstart

#pragma once

#include "analyzer/protocol/tcp/TCP.h"
#include "ID.h"

namespace binpac { namespace MQTT { class MQTT_Conn; } }

namespace analyzer { namespace MQTT {

class MQTT_Analyzer final : public tcp::TCP_ApplicationAnalyzer {

public:
	MQTT_Analyzer(Connection* conn);
	~MQTT_Analyzer() override;

	void Done() override;
	void DeliverStream(int len, const u_char* data, bool orig) override;
	void Undelivered(uint64_t seq, int len, bool orig) override;
	void EndpointEOF(bool is_orig) override;

	static analyzer::Analyzer* InstantiateAnalyzer(Connection* conn)
		{ return new MQTT_Analyzer(conn); }

protected:
	binpac::MQTT::MQTT_Conn* interp;

};

} } // namespace analyzer::*