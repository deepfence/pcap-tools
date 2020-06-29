%include binpac.pac
%include bro.pac

%extern{
#include "IP.h"
#include "TunnelEncapsulation.h"
#include "Reporter.h"
#include "events.bif.h"
%}

analyzer GTPv1 withcontext {
	connection:	GTPv1_Conn;
	flow:		GTPv1_Flow;
};

%include gtpv1-protocol.pac
%include gtpv1-analyzer.pac
