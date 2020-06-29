# Analyzer for SSL messages (general part).
# To be used in conjunction with an SSL record-layer analyzer.
# Separation is necessary due to possible fragmentation of SSL records.

type SSLRecord(is_orig: bool) = record {
	head0 : uint8;
	head1 : uint8;
	head2 : uint8;
	head3 : uint8;
	head4 : uint8;
	rec : RecordText(this)[] &length=length, &requires(version,content_type,raw_tls_version);
} &length = length+5, &byteorder=bigendian,
	&let {
	version : int =
		$context.connection.determine_ssl_record_layer(head0, head1, head2, head3, head4, is_orig);

	# unmodified tls record layer version of this packet. Do not use this if you are parsing SSLv2
	raw_tls_version: uint16 = case version of {
		SSLv20 -> 0;
		default -> (head1<<8) | head2;
	} &requires(version);

	content_type : int = case version of {
		SSLv20 -> head2+300;
		default -> head0;
	} &requires(version);

	length : int = case version of {
		# fail analyzer if the packet cannot be recognized as TLS.
		UNKNOWN_VERSION -> 0;
		SSLv20 -> (((head0 & 0x7f) << 8) | head1) - 3;
		default -> (head3 << 8) | head4;
	};
};

type RecordText(rec: SSLRecord) = case $context.connection.state(rec.is_orig) of {
	STATE_ENCRYPTED
		-> ciphertext : CiphertextRecord(rec);
	default
		-> plaintext : PlaintextRecord(rec);
};

refine casetype PlaintextRecord += {
	HANDSHAKE		-> handshake : Handshake(rec);
	V2_ERROR		-> v2_error : V2Error(rec);
	V2_CLIENT_HELLO		-> v2_client_hello : V2ClientHello(rec);
	V2_CLIENT_MASTER_KEY	-> v2_client_master_key : V2ClientMasterKey(rec);
	V2_SERVER_HELLO		-> v2_server_hello : V2ServerHello(rec);
};

# Handshakes are parsed by the handshake analyzer.
type Handshake(rec: SSLRecord) = record {
	data: bytestring &restofdata;
};

######################################################################
# V2 Error Records (SSLv2 2.7.)
######################################################################

type V2Error(rec: SSLRecord) = record {
	data : bytestring &restofdata &transient;
} &let {
	error_code : uint16 = ((rec.head3 << 8) | rec.head4);
};



######################################################################
# V2 Client Hello (SSLv2 2.5.)
######################################################################

type V2ClientHello(rec: SSLRecord) = record {
	csuit_len : uint16;
	session_len : uint16;
	chal_len : uint16;
	ciphers : uint24[csuit_len/3];
	session_id : uint8[session_len];
	challenge : bytestring &length = chal_len;
} &length = 6 + csuit_len + session_len + chal_len, &let {
	client_version : int = rec.version;
};



######################################################################
# V2 Server Hello (SSLv2 2.6.)
######################################################################

type V2ServerHello(rec: SSLRecord) = record {
	#session_id_hit : uint8;
	#cert_type : uint8;
	server_version : uint16;
	cert_len : uint16;
	ciph_len : uint16;
	conn_id_len : uint16;
	cert_data : bytestring &length = cert_len;
	ciphers : uint24[ciph_len/3];
	conn_id_data : bytestring &length = conn_id_len;
} &let {
	session_id_hit : uint8 = rec.head3;
	cert_type : uint8 = rec.head4;
};


######################################################################
# V2 Client Master Key (SSLv2 2.5.)
######################################################################

type V2ClientMasterKey(rec: SSLRecord) = record {
	cipher_kind_8 : uint8;
	cl_key_len : uint16;
	en_key_len : uint16;
	key_arg_len : uint16;
	cl_key_data : bytestring &length = cl_key_len &transient;
	en_key_data : bytestring &length = en_key_len &transient;
	key_arg_data : bytestring &length = key_arg_len &transient;
} &length = 7 + cl_key_len + en_key_len + key_arg_len, &let {
	cipher_kind : int = (((rec.head3 << 16) | (rec.head4 << 8)) | cipher_kind_8);
	# encryption starts for both sides after this message.
	state_changed_client : bool = $context.connection.startEncryption(true);
	state_changed_server : bool = $context.connection.startEncryption(false);
};


######################################################################
# initial datatype for binpac
######################################################################

type SSLPDU(is_orig: bool) = record {
	records : SSLRecord(is_orig)[] &transient;
} &byteorder = bigendian;


######################################################################
# binpac analyzer for SSL including
######################################################################

refine connection SSL_Conn += {

	function determine_ssl_record_layer(head0 : uint8, head1 : uint8,
					head2 : uint8, head3: uint8, head4: uint8, is_orig: bool) : int
		%{
		// stop processing if we already had a protocol violation or otherwhise
		// decided that we do not want to parse anymore. Just setting skip is not
		// enough for the data that is already in the pipe.
		if ( bro_analyzer()->Skipping() )
			return UNKNOWN_VERSION;

		// re-check record layer version to be sure that we still are synchronized with
		// the data stream
		if ( record_layer_version_ != UNKNOWN_VERSION && record_layer_version_ != SSLv20 )
			{
			uint16 version = (head1<<8) | head2;
			if ( version != SSLv30 && version != TLSv10 &&
			     version != TLSv11 && version != TLSv12 )
				{
				bro_analyzer()->ProtocolViolation(fmt("Invalid version late in TLS connection. Packet reported version: %d", version));
				bro_analyzer()->SetSkip(true);
				return UNKNOWN_VERSION;
				}
			}

		if ( record_layer_version_ != UNKNOWN_VERSION )
			return record_layer_version_;

		if ( head0 & 0x80 )
			{
			if ( head2 == 0x01 && is_orig ) // SSLv2 client hello.
				{
				uint16 version = (head3 << 8) | head4;
				if ( version != SSLv20 && version != SSLv30 && version != TLSv10 &&
				     version != TLSv11 && version != TLSv12 )
					{
					bro_analyzer()->ProtocolViolation(fmt("Invalid version in SSL client hello. Version: %d", version));
					bro_analyzer()->SetSkip(true);
					return UNKNOWN_VERSION;
					}

				else
					return SSLv20;
				}

			else if ( head2 == 0x04 && head4 < 2 && ! is_orig ) // SSLv2 server hello. This connection will continue using SSLv2.
				{
				record_layer_version_ = SSLv20;
				return SSLv20;
				}

			else // this is not SSL or TLS.
				{
				bro_analyzer()->ProtocolViolation(fmt("Invalid headers in SSL connection. Head1: %d, head2: %d, head3: %d", head1, head2, head3));
				bro_analyzer()->SetSkip(true);
				return UNKNOWN_VERSION;
				}
			}

		uint16 version = (head1<<8) | head2;
		if ( version != SSLv30 && version != TLSv10 &&
		     version != TLSv11 && version != TLSv12 )
			{
			bro_analyzer()->ProtocolViolation(fmt("Invalid version in TLS connection. Version: %d", version));
			bro_analyzer()->SetSkip(true);
			return UNKNOWN_VERSION;
			}

		if ( head0 >=20 && head0 <= 30 )
			{ // ok, set record layer version, this never can be downgraded to v2
			record_layer_version_ = version;
			return version;
			}

		bro_analyzer()->ProtocolViolation(fmt("Invalid type in TLS connection. Version: %d, Type: %d", version, head0));
		bro_analyzer()->SetSkip(true);
		return UNKNOWN_VERSION;
		%}

	function record_version() : uint16 %{ return 0; %}

};
