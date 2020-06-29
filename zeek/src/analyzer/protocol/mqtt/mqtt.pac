# Analyzer for MQTT Protocol (currently v3.1.1, no v5.0 support)

%include binpac.pac
%include bro.pac

%extern{
	#include "MQTT.h"
	#include "events.bif.h"
	#include "types.bif.h"
%}

analyzer MQTT withcontext {
	connection: MQTT_Conn;
	flow:       MQTT_Flow;
};

# Our connection consists of two flows, one in each direction.
connection MQTT_Conn(bro_analyzer: BroAnalyzer) {
	upflow   = MQTT_Flow(true);
	downflow = MQTT_Flow(false);
};

%include mqtt-protocol.pac

flow MQTT_Flow(is_orig: bool) {
	#flowunit = MQTT_PDU(is_orig) withcontext(connection, this);
	datagram = MQTT_PDU(is_orig) withcontext(connection, this);
};

%include commands/connect.pac
%include commands/connack.pac
%include commands/publish.pac
%include commands/puback.pac
%include commands/pubrec.pac
%include commands/pubrel.pac
%include commands/pubcomp.pac
%include commands/subscribe.pac
%include commands/suback.pac
%include commands/unsuback.pac
%include commands/unsubscribe.pac
%include commands/disconnect.pac
%include commands/pingreq.pac
%include commands/pingresp.pac

