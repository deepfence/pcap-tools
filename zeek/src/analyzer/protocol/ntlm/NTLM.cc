// See the file "COPYING" in the main distribution directory for copyright.

#include "NTLM.h"
#include "analyzer/protocol/tcp/TCP_Reassembler.h"
#include "Reporter.h"
#include "events.bif.h"

using namespace analyzer::ntlm;

NTLM_Analyzer::NTLM_Analyzer(Connection* c)
	: tcp::TCP_ApplicationAnalyzer("NTLM", c)
	{
	interp = new binpac::NTLM::NTLM_Conn(this);
	}

NTLM_Analyzer::~NTLM_Analyzer()
	{
	delete interp;
	}

void NTLM_Analyzer::Done()
	{
	tcp::TCP_ApplicationAnalyzer::Done();

	interp->FlowEOF(true);
	interp->FlowEOF(false);
	}

void NTLM_Analyzer::EndpointEOF(bool is_orig)
	{
	tcp::TCP_ApplicationAnalyzer::EndpointEOF(is_orig);
	interp->FlowEOF(is_orig);
	}

void NTLM_Analyzer::DeliverStream(int len, const u_char* data, bool orig)
	{
	tcp::TCP_ApplicationAnalyzer::DeliverStream(len, data, orig);

	assert(TCP());

	try
		{
		interp->NewData(orig, data, data + len);
		ProtocolConfirmation();
		}
	catch ( const binpac::Exception& e )
		{
		ProtocolViolation(fmt("Binpac exception: %s", e.c_msg()));
		}
	}

void NTLM_Analyzer::Undelivered(uint64_t seq, int len, bool orig)
	{
	tcp::TCP_ApplicationAnalyzer::Undelivered(seq, len, orig);
	interp->NewGap(orig, len);
	}
