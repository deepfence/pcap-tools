@load base/frameworks/tunnels
@load ./consts

module SOCKS;

export {
	redef enum Log::ID += { LOG };

	## Whether passwords are captured or not.
	option default_capture_password = F;

	## The record type which contains the fields of the SOCKS log.
	type Info: record {
		## Time when the proxy connection was first detected.
		ts:               time            &log;
		## Unique ID for the tunnel - may correspond to connection uid
		## or be non-existent.
		uid:              string          &log;
		## The connection's 4-tuple of endpoint addresses/ports.
		id:               conn_id         &log;
		## Protocol version of SOCKS.
		version:          count           &log;
		## Username used to request a login to the proxy.
		user:             string          &log &optional;
		## Password used to request a login to the proxy.
		password:         string          &log &optional;
		## Server status for the attempt at using the proxy.
		status:           string          &log &optional;
		## Client requested SOCKS address. Could be an address, a name
		## or both.
		request:          SOCKS::Address  &log &optional;
		## Client requested port.
		request_p:        port            &log &optional;
		## Server bound address. Could be an address, a name or both.
		bound:            SOCKS::Address  &log &optional;
		## Server bound port.
		bound_p:          port            &log &optional;
		## Determines if the password will be captured for this request.
		capture_password: bool            &default=default_capture_password;
	};

	## Event that can be handled to access the SOCKS
	## record as it is sent on to the logging framework.
	global log_socks: event(rec: Info);
}

const ports = { 1080/tcp };
redef likely_server_ports += { ports };

event zeek_init() &priority=5
	{
	Log::create_stream(SOCKS::LOG, [$columns=Info, $ev=log_socks, $path="socks"]);
	Analyzer::register_for_ports(Analyzer::ANALYZER_SOCKS, ports);
	}

redef record connection += {
	socks: SOCKS::Info &optional;
};

function set_session(c: connection, version: count)
	{
	if ( ! c?$socks )
		c$socks = [$ts=network_time(), $id=c$id, $uid=c$uid, $version=version];
	}

event socks_request(c: connection, version: count, request_type: count,
                    sa: SOCKS::Address, p: port, user: string) &priority=5
	{
	set_session(c, version);

	c$socks$request   = sa;
	c$socks$request_p = p;

	# Copy this conn_id and set the orig_p to zero because in the case of SOCKS proxies there will
	# be potentially many source ports since a new proxy connection is established for each
	# proxied connection.  We treat this as a singular "tunnel".
	local cid = copy(c$id);
	cid$orig_p = 0/tcp;
	Tunnel::register([$cid=cid, $tunnel_type=Tunnel::SOCKS]);
	}

event socks_reply(c: connection, version: count, reply: count, sa: SOCKS::Address, p: port) &priority=5
	{
	set_session(c, version);

	if ( version == 5 )
		c$socks$status = v5_status[reply];
	else if ( version == 4 )
		c$socks$status = v4_status[reply];

	c$socks$bound   = sa;
	c$socks$bound_p = p;
	}

event socks_login_userpass_request(c: connection, user: string, password: string) &priority=5
	{
	# Authentication only possible with the version 5.
	set_session(c, 5);

	c$socks$user = user;

	if ( c$socks$capture_password )
		c$socks$password = password;
	}

event socks_login_userpass_reply(c: connection, code: count) &priority=5
	{
	# Authentication only possible with the version 5.
	set_session(c, 5);

	c$socks$status = v5_status[code];
	}

event successful_connection_remove(c: connection)
	{
	# This will handle the case where the analyzer failed in some way and was
	# removed.  We probably  don't want to log these connections.
	if ( "SOCKS" in c$service )
		Log::write(SOCKS::LOG, c$socks);
	}
