#include "binpac.h"

#include "Net.h"
#include "Conn.h"
#include "Sessions.h"
#include "analyzer/Analyzer.h"
#include "analyzer/Manager.h"
#include "analyzer/protocol/pia/PIA.h"
#include "analyzer/protocol/tcp/TCP.h"

#include "FuzzBuffer.h"
#include "fuzzer-setup.h"

static constexpr auto ZEEK_FUZZ_ANALYZER = "pop3";

static Connection* add_connection()
	{
	static constexpr double network_time_start = 1439471031;
	net_update_time(network_time_start);

	Packet p;
	ConnID conn_id;
	conn_id.src_addr = IPAddr("1.2.3.4");
	conn_id.dst_addr = IPAddr("5.6.7.8");
	conn_id.src_port = htons(23132);
	conn_id.dst_port = htons(80);
	ConnIDKey key = BuildConnIDKey(conn_id);
	Connection* conn = new Connection(sessions, key, network_time_start,
	                                  &conn_id, 1, &p, nullptr);
	conn->SetTransport(TRANSPORT_TCP);
	sessions->Insert(conn);
	return conn;
	}

static analyzer::Analyzer* add_analyzer(Connection* conn)
	{
	analyzer::tcp::TCP_Analyzer* tcp = new analyzer::tcp::TCP_Analyzer(conn);
	analyzer::pia::PIA* pia = new analyzer::pia::PIA_TCP(conn);
	auto a = analyzer_mgr->InstantiateAnalyzer(ZEEK_FUZZ_ANALYZER, conn);
	tcp->AddChildAnalyzer(a);
	tcp->AddChildAnalyzer(pia->AsAnalyzer());
	conn->SetRootAnalyzer(tcp, pia);
	return a;
	}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
	{
	zeek::detail::FuzzBuffer fb{data, size};

	if ( ! fb.Valid() )
		return 0;

	auto conn = add_connection();
	auto a = add_analyzer(conn);

	for ( ; ;  )
		{
		auto chunk = fb.Next();

		if ( ! chunk )
			break;

		try
			{
			a->DeliverStream(chunk->size, chunk->data.get(), chunk->is_orig);
			}
		catch ( const binpac::Exception& e )
			{
			}

		chunk = {};
		mgr.Drain();
		}

	zeek::detail::fuzzer_cleanup_one_input();
	return 0;
	}
