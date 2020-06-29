##! This script provides a default set of actions to take for "weird activity"
##! events generated from Zeek's event engine.  Weird activity is defined as
##! unusual or exceptional activity that can indicate malformed connections,
##! traffic that doesn't conform to a particular protocol, malfunctioning
##! or misconfigured hardware, or even an attacker attempting to avoid/confuse
##! a sensor.  Without context, it's hard to judge whether a particular
##! category of weird activity is interesting, but this script provides
##! a starting point for the user.

@load base/utils/conn-ids
@load base/utils/site
@load ./main

module Weird;

export {
	## The weird logging stream identifier.
	redef enum Log::ID += { LOG };

	redef enum Notice::Type += {
		## Generic unusual but notice-worthy weird activity.
		Activity,
	};

	## The record which is used for representing and logging weirds.
	type Info: record {
		## The time when the weird occurred.
		ts:     time    &log;

		## If a connection is associated with this weird, this will be
		## the connection's unique ID.
		uid:    string  &log &optional;

		## conn_id for the optional connection.
		id:     conn_id &log &optional;

		## A shorthand way of giving the uid and id to a weird.
		conn:   connection &optional;

		## The name of the weird that occurred.
		name:   string  &log;

		## Additional information accompanying the weird if any.
		addl:   string  &log &optional;

		## Indicate if this weird was also turned into a notice.
		notice:  bool   &log &default=F;

		## The peer that originated this weird.  This is helpful in
		## cluster deployments if a particular cluster node is having
		## trouble to help identify which node is having trouble.
		peer:   string  &log &optional &default=peer_description;

		## This field is to be provided when a weird is generated for
		## the purpose of deduplicating weirds. The identifier string
		## should be unique for a single instance of the weird. This field
		## is used to define when a weird is conceptually a duplicate of
		## a previous weird.
		identifier: string &optional;
	};

	## Types of actions that may be taken when handling weird activity events.
	type Action: enum {
		## A dummy action indicating the user does not care what
		## internal decision is made regarding a given type of weird.
		ACTION_UNSPECIFIED,
		## No action is to be taken.
		ACTION_IGNORE,
		## Log the weird event every time it occurs.
		ACTION_LOG,
		## Log the weird event only once.
		ACTION_LOG_ONCE,
		## Log the weird event once per connection.
		ACTION_LOG_PER_CONN,
		## Log the weird event once per originator host.
		ACTION_LOG_PER_ORIG,
		## Always generate a notice associated with the weird event.
		ACTION_NOTICE,
		## Generate a notice associated with the weird event only once.
		ACTION_NOTICE_ONCE,
		## Generate a notice for the weird event once per connection.
		ACTION_NOTICE_PER_CONN,
		## Generate a notice for the weird event once per originator host.
		ACTION_NOTICE_PER_ORIG,
	};

	## A table specifying default/recommended actions per weird type.
	const actions: table[string] of Action = {
		["unsolicited_SYN_response"]            = ACTION_IGNORE,
		["above_hole_data_without_any_acks"]    = ACTION_LOG,
		["active_connection_reuse"]             = ACTION_LOG,
		["bad_HTTP_reply"]                      = ACTION_LOG,
		["bad_HTTP_version"]                    = ACTION_LOG,
		["bad_ICMP_checksum"]                   = ACTION_LOG_PER_ORIG,
		["bad_ident_port"]                      = ACTION_LOG,
		["bad_ident_reply"]                     = ACTION_LOG,
		["bad_ident_request"]                   = ACTION_LOG,
		["bad_rlogin_prolog"]                   = ACTION_LOG,
		["bad_rsh_prolog"]                      = ACTION_LOG,
		["rsh_text_after_rejected"]             = ACTION_LOG,
		["bad_RPC"]                             = ACTION_LOG_PER_ORIG,
		["bad_RPC_program"]                     = ACTION_LOG,
		["bad_SYN_ack"]                         = ACTION_LOG,
		["bad_TCP_checksum"]                    = ACTION_LOG_PER_ORIG,
		["bad_UDP_checksum"]                    = ACTION_LOG_PER_ORIG,
		["baroque_SYN"]                         = ACTION_LOG,
		["base64_illegal_encoding"]             = ACTION_LOG,
		["connection_originator_SYN_ack"]       = ACTION_LOG_PER_ORIG,
		["contentline_size_exceeded"]           = ACTION_LOG,
		["crud_trailing_HTTP_request"]          = ACTION_LOG,
		["data_after_reset"]                    = ACTION_LOG,
		["data_before_established"]             = ACTION_LOG,
		["DNS_AAAA_neg_length"]                 = ACTION_LOG,
		["DNS_Conn_count_too_large"]            = ACTION_LOG,
		["DNS_NAME_too_long"]                   = ACTION_LOG,
		["DNS_RR_bad_length"]                   = ACTION_LOG,
		["DNS_RR_length_mismatch"]              = ACTION_LOG,
		["DNS_RR_unknown_type"]                 = ACTION_LOG,
		["DNS_label_forward_compress_offset"]   = ACTION_LOG_PER_ORIG,
		["DNS_label_len_gt_name_len"]           = ACTION_LOG_PER_ORIG,
		["DNS_label_len_gt_pkt"]                = ACTION_LOG_PER_ORIG,
		["DNS_label_too_long"]                  = ACTION_LOG_PER_ORIG,
		["DNS_truncated_RR_rdlength_lt_len"]    = ACTION_LOG,
		["DNS_truncated_ans_too_short"]         = ACTION_LOG,
		["DNS_truncated_len_lt_hdr_len"]        = ACTION_LOG,
		["DNS_truncated_quest_too_short"]       = ACTION_LOG,
		["excessive_data_without_further_acks"] = ACTION_LOG,
		["excess_RPC"]                          = ACTION_LOG_PER_ORIG,
		["FIN_advanced_last_seq"]               = ACTION_LOG,
		["FIN_after_reset"]                     = ACTION_IGNORE,
		["FIN_storm"]                           = ACTION_NOTICE_PER_ORIG,
		["HTTP_bad_chunk_size"]                 = ACTION_LOG,
		["HTTP_chunked_transfer_for_multipart_message"] = ACTION_LOG,
		["HTTP_overlapping_messages"]           = ACTION_LOG,
		["unknown_HTTP_method"]                 = ACTION_LOG,
		["HTTP_version_mismatch"]               = ACTION_LOG,
		["ident_request_addendum"]              = ACTION_LOG,
		["inappropriate_FIN"]                   = ACTION_LOG,
		["inflate_failed"]                      = ACTION_LOG,
		["invalid_irc_global_users_reply"]      = ACTION_LOG,
		["irc_invalid_command"]                 = ACTION_LOG,
		["irc_invalid_dcc_message_format"]      = ACTION_LOG,
		["irc_invalid_invite_message_format"]   = ACTION_LOG,
		["irc_invalid_join_line"]               = ACTION_LOG,
		["irc_invalid_kick_message_format"]     = ACTION_LOG,
		["irc_invalid_line"]                    = ACTION_LOG,
		["irc_invalid_mode_message_format"]     = ACTION_LOG,
		["irc_invalid_names_line"]              = ACTION_LOG,
		["irc_invalid_njoin_line"]              = ACTION_LOG,
		["irc_invalid_notice_message_format"]   = ACTION_LOG,
		["irc_invalid_oper_message_format"]     = ACTION_LOG,
		["irc_invalid_privmsg_message_format"]  = ACTION_LOG,
		["irc_invalid_reply_number"]            = ACTION_LOG,
		["irc_invalid_squery_message_format"]   = ACTION_LOG,
		["irc_invalid_topic_reply"]             = ACTION_LOG,
		["irc_invalid_who_line"]                = ACTION_LOG,
		["irc_invalid_who_message_format"]      = ACTION_LOG,
		["irc_invalid_whois_channel_line"]      = ACTION_LOG,
		["irc_invalid_whois_message_format"]    = ACTION_LOG,
		["irc_invalid_whois_operator_line"]     = ACTION_LOG,
		["irc_invalid_whois_user_line"]         = ACTION_LOG,
		["irc_line_size_exceeded"]              = ACTION_LOG,
		["irc_line_too_short"]                  = ACTION_LOG,
		["irc_too_many_invalid"]                = ACTION_LOG,
		["line_terminated_with_single_CR"]      = ACTION_LOG,
		["line_terminated_with_single_LF"]      = ACTION_LOG,
		["malformed_ssh_identification"]        = ACTION_LOG,
		["malformed_ssh_version"]               = ACTION_LOG,
		["multiple_HTTP_request_elements"]      = ACTION_LOG,
		["NUL_in_line"]                         = ACTION_LOG,
		["originator_RPC_reply"]                = ACTION_LOG_PER_ORIG,
		["partial_finger_request"]              = ACTION_LOG,
		["partial_ftp_request"]                 = ACTION_LOG,
		["partial_ident_request"]               = ACTION_LOG,
		["partial_RPC"]                         = ACTION_LOG_PER_ORIG,
		["pending_data_when_closed"]            = ACTION_LOG,
		["pop3_bad_base64_encoding"]            = ACTION_LOG,
		["pop3_client_command_unknown"]         = ACTION_LOG,
		["pop3_client_sending_server_commands"] = ACTION_LOG,
		["pop3_malformed_auth_plain"]           = ACTION_LOG,
		["pop3_server_command_unknown"]         = ACTION_LOG,
		["pop3_server_sending_client_commands"] = ACTION_LOG,
		["possible_split_routing"]              = ACTION_LOG,
		["premature_connection_reuse"]          = ACTION_LOG,
		["repeated_SYN_reply_wo_ack"]           = ACTION_LOG,
		["repeated_SYN_with_ack"]               = ACTION_LOG,
		["responder_RPC_call"]                  = ACTION_LOG_PER_ORIG,
		["rlogin_text_after_rejected"]          = ACTION_LOG,
		["RPC_rexmit_inconsistency"]            = ACTION_LOG,
		["RPC_underflow"]                       = ACTION_LOG,
		["RST_storm"]                           = ACTION_LOG,
		["RST_with_data"]                       = ACTION_LOG,
		["SSL_many_server_names"]               = ACTION_LOG,
		["simultaneous_open"]                   = ACTION_LOG_PER_CONN,
		["spontaneous_FIN"]                     = ACTION_IGNORE,
		["spontaneous_RST"]                     = ACTION_IGNORE,
		["SMB_parsing_error"]                   = ACTION_LOG,
		["no_smb_session_using_parsesambamsg"]  = ACTION_LOG,
		["smb_andx_command_failed_to_parse"]    = ACTION_LOG,
		["transaction_subcmd_missing"]          = ACTION_LOG,
		["successful_RPC_reply_to_invalid_request"] = ACTION_NOTICE_PER_ORIG,
		["SYN_after_close"]                     = ACTION_LOG,
		["SYN_after_partial"]                   = ACTION_NOTICE_PER_ORIG,
		["SYN_after_reset"]                     = ACTION_LOG,
		["SYN_inside_connection"]               = ACTION_LOG,
		["SYN_seq_jump"]                        = ACTION_LOG,
		["SYN_with_data"]                       = ACTION_LOG_PER_ORIG,
		["TCP_christmas"]                       = ACTION_LOG,
		["truncated_ARP"]                       = ACTION_LOG,
		["truncated_NTP"]                       = ACTION_LOG,
		["UDP_datagram_length_mismatch"]        = ACTION_LOG_PER_ORIG,
		["unexpected_client_HTTP_data"]         = ACTION_LOG,
		["unexpected_multiple_HTTP_requests"]   = ACTION_LOG,
		["unexpected_server_HTTP_data"]         = ACTION_LOG,
		["unmatched_HTTP_reply"]                = ACTION_LOG,
		["unpaired_RPC_response"]               = ACTION_LOG,
		["window_recision"]                     = ACTION_LOG,
		["double_%_in_URI"]                     = ACTION_LOG,
		["illegal_%_at_end_of_URI"]             = ACTION_LOG,
		["unescaped_%_in_URI"]                  = ACTION_LOG,
		["unescaped_special_URI_char"]          = ACTION_LOG,
		["deficit_netbios_hdr_len"]             = ACTION_LOG,
		["excess_netbios_hdr_len"]              = ACTION_LOG,
		["netbios_client_session_reply"]        = ACTION_LOG,
		["netbios_raw_session_msg"]             = ACTION_LOG,
		["netbios_server_session_request"]      = ACTION_LOG,
		["unknown_netbios_type"]                = ACTION_LOG,
		["excessively_large_fragment"]          = ACTION_LOG,
		["excessively_small_fragment"]          = ACTION_LOG_PER_ORIG,
		["fragment_inconsistency"]              = ACTION_LOG_PER_ORIG,
		["fragment_overlap"]                    = ACTION_LOG_PER_ORIG,
		["fragment_protocol_inconsistency"]     = ACTION_LOG,
		["fragment_size_inconsistency"]         = ACTION_LOG_PER_ORIG,
		# These do indeed happen!
		["fragment_with_DF"]                    = ACTION_LOG,
		["incompletely_captured_fragment"]      = ACTION_LOG,
		["bad_IP_checksum"]                     = ACTION_LOG_PER_ORIG,
		["bad_TCP_header_len"]                  = ACTION_LOG,
		["internally_truncated_header"]         = ACTION_LOG,
		["truncated_IP"]                        = ACTION_LOG,
		["truncated_header"]                    = ACTION_LOG,
	} &default=ACTION_LOG &redef;

	## To completely ignore a specific weird for a host, add the host
	## and weird name into this set.
	option ignore_hosts: set[addr, string] = {};

	## Don't ignore repeats for weirds in this set.  For example,
	## it's handy keeping track of clustered checksum errors.
	option weird_do_not_ignore_repeats = {
		"bad_IP_checksum", "bad_TCP_checksum", "bad_UDP_checksum",
		"bad_ICMP_checksum",
	};

	## This table is used to track identifier and name pairs that should be
	## temporarily ignored because the problem has already been reported.
	## This helps reduce the volume of high volume weirds by only allowing 
	## a unique weird every ``create_expire`` interval.
	global weird_ignore: set[string, string] &create_expire=10min &redef;

	## A state set which tracks unique weirds solely by name to reduce
	## duplicate logging.  This is deliberately not synchronized because it
	## could cause overload during storms.
	global did_log: set[string, string] &create_expire=1day &redef;

	## A state set which tracks unique weirds solely by name to reduce
	## duplicate notices from being raised.
	global did_notice: set[string, string] &create_expire=1day &redef;

	## Handlers of this event are invoked once per write to the weird
	## logging stream before the data is actually written.
	##
	## rec: The weird columns about to be logged to the weird stream.
	global log_weird: event(rec: Info);

	global weird: function(w: Weird::Info);
}

# These actions result in the output being limited and further redundant
# weirds not progressing to being logged or noticed.
const limiting_actions = {
	ACTION_LOG_ONCE,
	ACTION_LOG_PER_CONN,
	ACTION_LOG_PER_ORIG,
	ACTION_NOTICE_ONCE,
	ACTION_NOTICE_PER_CONN,
	ACTION_NOTICE_PER_ORIG,
};

# This is an internal set to track which Weird::Action values lead to notice
# creation.
const notice_actions = {
	ACTION_NOTICE,
	ACTION_NOTICE_PER_CONN,
	ACTION_NOTICE_PER_ORIG,
	ACTION_NOTICE_ONCE,
};

event zeek_init() &priority=5
	{
	Log::create_stream(Weird::LOG, [$columns=Info, $ev=log_weird, $path="weird"]);
	}

function flow_id_string(src: addr, dst: addr): string
	{
	return fmt("%s -> %s", src, dst);
	}

function weird(w: Weird::Info)
	{
	local action = actions[w$name];

	local identifier = "";
	if ( w?$identifier )
		identifier = w$identifier;
	else
		{
		if ( w?$id )
			identifier = id_string(w$id);
		}

	# If this weird is to be ignored let's drop out of here very early.
	if ( action == ACTION_IGNORE || [w$name, identifier] in weird_ignore )
		return;

	if ( w?$conn )
		{
		w$uid = w$conn$uid;
		w$id = w$conn$id;
		}

	if ( w?$id )
		{
		if ( [w$id$orig_h, w$name] in ignore_hosts ||
				 [w$id$resp_h, w$name] in ignore_hosts )
				 return;
		}

	if ( action in limiting_actions )
		{
		local notice_identifier = identifier;
		if ( action in notice_actions )
			{
			# Handle notices
			if ( w?$id && action == ACTION_NOTICE_PER_ORIG )
				notice_identifier = fmt("%s", w$id$orig_h);
			else if ( action == ACTION_NOTICE_ONCE )
				notice_identifier = "";

			# If this weird was already noticed then we're done.
			if ( [w$name, notice_identifier] in did_notice )
				return;
			add did_notice[w$name, notice_identifier];
			}
		else
			{
			# Handle logging.
			if ( w?$id && action == ACTION_LOG_PER_ORIG )
				notice_identifier = fmt("%s", w$id$orig_h);
			else if ( action == ACTION_LOG_ONCE )
				notice_identifier = "";

			# If this weird was already logged then we're done.
			if ( [w$name, notice_identifier] in did_log )
				return;

			add did_log[w$name, notice_identifier];
			}
		}

	if ( action in notice_actions )
		{
		w$notice = T;

		local n: Notice::Info;
		n$note = Activity;
		n$msg = w$name;
		if ( w?$conn )
			n$conn = w$conn;
		else
			{
			if ( w?$uid )
				n$uid = w$uid;
			if ( w?$id )
				n$id = w$id;
			}
		if ( w?$addl )
			n$sub = w$addl;
		NOTICE(n);
		}

	# This is for the temporary ignoring to reduce volume for identical weirds.
	if ( w$name !in weird_do_not_ignore_repeats )
		add weird_ignore[w$name, identifier];

	Log::write(Weird::LOG, w);
	}

# The following events come from core generated weirds typically.
event conn_weird(name: string, c: connection, addl: string)
	{
	local i = Info($ts=network_time(), $name=name, $conn=c, $identifier=id_string(c$id));
	if ( addl != "" )
		i$addl = addl;

	weird(i);
	}

event expired_conn_weird(name: string, id: conn_id, uid: string, addl: string)
	{
	local i = Info($ts=network_time(), $name=name, $uid=uid, $id=id,
	               $identifier=id_string(id));

	if ( addl != "" )
		i$addl = addl;

	weird(i);
	}

event flow_weird(name: string, src: addr, dst: addr, addl: string)
	{
	# We add the source and destination as port 0/unknown because that is
	# what fits best here.
	local id = conn_id($orig_h=src, $orig_p=count_to_port(0, unknown_transport),
	                   $resp_h=dst, $resp_p=count_to_port(0, unknown_transport));

	local i = Info($ts=network_time(), $name=name, $id=id, $identifier=flow_id_string(src,dst));

	if ( addl != "" )
		i$addl = addl;

	weird(i);
	}

event net_weird(name: string, addl: string)
	{
	local i = Info($ts=network_time(), $name=name);

	if ( addl != "" )
		i$addl = addl;

	weird(i);
	}

event file_weird(name: string, f: fa_file, addl: string)
	{
	local i = Info($ts=network_time(), $name=name, $addl=f$id);

	if ( addl != "" )
		i$addl += fmt(": %s", addl);

	weird(i);
	}
