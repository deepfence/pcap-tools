%extern{
#include <cstdlib>
#include <vector>
#include <string>
%}

%header{
IntrusivePtr<VectorVal> name_list_to_vector(const bytestring& nl);
%}

%code{
// Copied from IRC_Analyzer::SplitWords
IntrusivePtr<VectorVal> name_list_to_vector(const bytestring& nl)
	{
	auto vv = make_intrusive<VectorVal>(zeek::id::string_vec);

	string name_list = std_str(nl);
	if ( name_list.size() < 1 )
		return vv;

	unsigned int start = 0;
	unsigned int split_pos = 0;

	while ( name_list[start] == ',' )
		{
		++start;
		++split_pos;
		}

	string word;
	while ( (split_pos = name_list.find(',', start)) < name_list.size() )
		{
		word = name_list.substr(start, split_pos - start);
		if ( word.size() > 0 && word[0] != ',' )
			vv->Assign(vv->Size(), make_intrusive<StringVal>(word));

		start = split_pos + 1;
		}

	// Add line end if needed.
	if ( start < name_list.size() )
		{
		word = name_list.substr(start, name_list.size() - start);
		vv->Assign(vv->Size(), make_intrusive<StringVal>(word));
		}
	return vv;
	}
%}

refine flow SSH_Flow += {
	function proc_ssh_version(msg: SSH_Version): bool
		%{
		if ( ssh_client_version && ${msg.is_orig } )
			{
			zeek::BifEvent::enqueue_ssh_client_version(connection()->bro_analyzer(),
				connection()->bro_analyzer()->Conn(),
				to_stringval(${msg.version}));
			}
		else if ( ssh_server_version )
			{
			zeek::BifEvent::enqueue_ssh_server_version(connection()->bro_analyzer(),
				connection()->bro_analyzer()->Conn(),
				to_stringval(${msg.version}));
			}
		return true;
		%}

	function proc_ssh2_kexinit(msg: SSH2_KEXINIT): bool
		%{
		if ( ! ssh_capabilities )
			return false;

		auto result = make_intrusive<RecordVal>(zeek::BifType::Record::SSH::Capabilities);
		result->Assign(0, name_list_to_vector(${msg.kex_algorithms.val}));
		result->Assign(1, name_list_to_vector(${msg.server_host_key_algorithms.val}));

		auto encryption_algs = make_intrusive<RecordVal>(zeek::BifType::Record::SSH::Algorithm_Prefs);
		encryption_algs->Assign(0, name_list_to_vector(${msg.encryption_algorithms_client_to_server.val}));
		encryption_algs->Assign(1, name_list_to_vector(${msg.encryption_algorithms_server_to_client.val}));
		result->Assign(2, std::move(encryption_algs));

		auto mac_algs = make_intrusive<RecordVal>(zeek::BifType::Record::SSH::Algorithm_Prefs);
		mac_algs->Assign(0, name_list_to_vector(${msg.mac_algorithms_client_to_server.val}));
		mac_algs->Assign(1, name_list_to_vector(${msg.mac_algorithms_server_to_client.val}));
		result->Assign(3, std::move(mac_algs));

		auto compression_algs = make_intrusive<RecordVal>(zeek::BifType::Record::SSH::Algorithm_Prefs);
		compression_algs->Assign(0, name_list_to_vector(${msg.compression_algorithms_client_to_server.val}));
		compression_algs->Assign(1, name_list_to_vector(${msg.compression_algorithms_server_to_client.val}));
		result->Assign(4, std::move(compression_algs));

		if ( ${msg.languages_client_to_server.len} || ${msg.languages_server_to_client.len} )
			{
			auto languages = make_intrusive<RecordVal>(zeek::BifType::Record::SSH::Algorithm_Prefs);
			if ( ${msg.languages_client_to_server.len} )
				languages->Assign(0, name_list_to_vector(${msg.languages_client_to_server.val}));
			if ( ${msg.languages_server_to_client.len} )
				languages->Assign(1, name_list_to_vector(${msg.languages_server_to_client.val}));

			result->Assign(5, std::move(languages));
			}


		result->Assign(6, val_mgr->Bool(!${msg.is_orig}));

		zeek::BifEvent::enqueue_ssh_capabilities(connection()->bro_analyzer(),
			connection()->bro_analyzer()->Conn(), to_stringval(${msg.cookie}),
			result);

		return true;
		%}


	function proc_ssh2_dh_gex_group(msg: SSH2_DH_GEX_GROUP): bool
		%{
		if ( ssh2_dh_server_params )
			{
			zeek::BifEvent::enqueue_ssh2_dh_server_params(connection()->bro_analyzer(),
				connection()->bro_analyzer()->Conn(),
				to_stringval(${msg.p.val}), to_stringval(${msg.g.val}));
			}
		return true;
		%}

	function proc_ssh2_ecc_key(q: bytestring, is_orig: bool): bool
		%{
		if ( ssh2_ecc_key )
			{
			zeek::BifEvent::enqueue_ssh2_ecc_key(connection()->bro_analyzer(),
				connection()->bro_analyzer()->Conn(),
				is_orig, to_stringval(q));
			}
		return true;
		%}

	function proc_ssh2_gss_error(msg: SSH2_GSS_ERROR): bool
		%{
		if ( ssh2_gss_error )
			{
			zeek::BifEvent::enqueue_ssh2_gss_error(connection()->bro_analyzer(),
				connection()->bro_analyzer()->Conn(),
				${msg.major_status}, ${msg.minor_status},
				to_stringval(${msg.message.val}));
			}
		return true;
		%}

	function proc_ssh2_server_host_key(key: bytestring): bool
		%{
		if ( ssh2_server_host_key )
			{
			zeek::BifEvent::enqueue_ssh2_server_host_key(connection()->bro_analyzer(),
				connection()->bro_analyzer()->Conn(),
				to_stringval(${key}));
			}
		return true;
		%}

	function proc_ssh1_server_host_key(p: bytestring, e: bytestring): bool
		%{
		if ( ssh1_server_host_key )
			{
			zeek::BifEvent::enqueue_ssh1_server_host_key(connection()->bro_analyzer(),
				connection()->bro_analyzer()->Conn(),
				to_stringval(${p}),
				to_stringval(${e}));
			}
		return true;
		%}

	function proc_newkeys(): bool
		%{
		connection()->bro_analyzer()->ProtocolConfirmation();
		return true;
		%}

	function get_kex_length(v: int, packet_length: uint32): int
		%{
		switch (v) {
			case SSH1:
				return packet_length + 4 + 8 - (packet_length % 8);
			case SSH2:
				return packet_length + 4;
			default:
				return 1; //currently causes the rest of the packet to dump
		}
		%}
};

refine typeattr SSH_Version += &let {
	proc: bool = $context.flow.proc_ssh_version(this);
};

refine typeattr SSH2_KEXINIT += &let {
	proc: bool = $context.flow.proc_ssh2_kexinit(this);
};

refine typeattr SSH1_Message += &let {
	proc_newkeys: bool = $context.flow.proc_newkeys() &if(msg_type == SSH_CMSG_SESSION_KEY);
};

refine typeattr SSH2_Message += &let {
	proc_newkeys: bool = $context.flow.proc_newkeys() &if(msg_type == MSG_NEWKEYS);
};

refine typeattr SSH2_DH_GEX_REPLY += &let {
	proc: bool = $context.flow.proc_ssh2_server_host_key(k_s.val);
};

refine typeattr SSH2_GSS_HOSTKEY += &let {
	proc: bool = $context.flow.proc_ssh2_server_host_key(k_s.val);
};

refine typeattr SSH2_GSS_ERROR += &let {
	proc: bool = $context.flow.proc_ssh2_gss_error(this);
};

refine typeattr SSH2_DH_GEX_GROUP += &let {
	proc: bool = $context.flow.proc_ssh2_dh_gex_group(this);
};

refine typeattr SSH2_ECC_REPLY += &let {
	proc_k: bool = $context.flow.proc_ssh2_server_host_key(k_s.val);
	proc_q: bool = $context.flow.proc_ssh2_ecc_key(q_s.val, false);
};

refine typeattr SSH2_ECC_INIT += &let {
	proc: bool = $context.flow.proc_ssh2_ecc_key(q_c.val, true);
};

refine typeattr SSH1_PUBLIC_KEY += &let {
	proc:  bool = $context.flow.proc_ssh1_server_host_key(host_key_p.val, host_key_e.val);
};
