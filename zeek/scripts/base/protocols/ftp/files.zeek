@load ./info
@load ./main
@load ./utils
@load base/utils/conn-ids
@load base/frameworks/files

module FTP;

export {
	redef record Info += {
		## File unique ID.
		fuid: string &optional &log;
	};

	## Default file handle provider for FTP.
	global get_file_handle: function(c: connection, is_orig: bool): string;

	## Describe the file being transferred.
	global describe_file: function(f: fa_file): string;

	redef record fa_file += { 
		ftp: FTP::Info &optional;
	};
}

function get_file_handle(c: connection, is_orig: bool): string
	{
	if ( [c$id$resp_h, c$id$resp_p] !in ftp_data_expected ) 
		return "";

	return cat(Analyzer::ANALYZER_FTP_DATA, c$start_time, c$id, is_orig);
	}

function describe_file(f: fa_file): string
	{
	# This shouldn't be needed, but just in case...
	if ( f$source != "FTP" )
		return "";

	for ( cid, c in f$conns )
		{
		if ( c?$ftp )
			return FTP::describe(c$ftp);
		}
	return "";
	}

event zeek_init() &priority=5
	{
	Files::register_protocol(Analyzer::ANALYZER_FTP_DATA,
	                         [$get_file_handle = FTP::get_file_handle,
	                          $describe        = FTP::describe_file]);
	}

event file_over_new_connection(f: fa_file, c: connection, is_orig: bool) &priority=5
	{
	if ( [c$id$resp_h, c$id$resp_p] !in ftp_data_expected ) 
		return;

	local ftp = ftp_data_expected[c$id$resp_h, c$id$resp_p];
	ftp$fuid = f$id;

	f$ftp = ftp;
	}

event file_sniff(f: fa_file, meta: fa_metadata) &priority=5
	{
	if ( ! f?$ftp )
		return;

	if ( ! meta?$mime_type )
		return;

	f$ftp$mime_type = meta$mime_type;
	}
