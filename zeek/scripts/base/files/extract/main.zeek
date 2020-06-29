@load base/frameworks/files
@load base/utils/paths

module FileExtract;

export {
	## The prefix where files are extracted to.
	const prefix = "./extract_files/" &redef;

	## The default max size for extracted files (they won't exceed this
	## number of bytes). A value of zero means unlimited.
	option default_limit = 0;

	redef record Files::Info += {
		## Local filename of extracted file.
		extracted: string &optional &log;

		## Set to true if the file being extracted was cut off
		## so the whole file was not logged.
		extracted_cutoff: bool &optional &log;

		## The number of bytes extracted to disk.
		extracted_size: count &optional &log;
	};

	redef record Files::AnalyzerArgs += {
		## The local filename to which to write an extracted file.
		## This field is used in the core by the extraction plugin
		## to know where to write the file to.  If not specified, then
		## a filename in the format "extract-<source>-<id>" is
		## automatically assigned (using the *source* and *id*
		## fields of :zeek:see:`fa_file`).
		extract_filename: string &optional;
		## The maximum allowed file size in bytes of *extract_filename*.
		## Once reached, a :zeek:see:`file_extraction_limit` event is
		## raised and the analyzer will be removed unless
		## :zeek:see:`FileExtract::set_limit` is called to increase the
		## limit.  A value of zero means "no limit".
		extract_limit: count &default=default_limit;
	};

	## Sets the maximum allowed extracted file size.
	##
	## f: A file that's being extracted.
	##
	## args: Arguments that identify a file extraction analyzer.
	##
	## n: Allowed number of bytes to be extracted.
	##
	## Returns: false if a file extraction analyzer wasn't active for
	##          the file, else true.
	global set_limit: function(f: fa_file, args: Files::AnalyzerArgs, n: count): bool;
}

function set_limit(f: fa_file, args: Files::AnalyzerArgs, n: count): bool
	{
	return __set_limit(f$id, args, n);
	}

function on_add(f: fa_file, args: Files::AnalyzerArgs)
	{
	if ( ! args?$extract_filename )
		args$extract_filename = cat("extract-", f$last_active, "-", f$source,
		                            "-", f$id);

	f$info$extracted = args$extract_filename;
	args$extract_filename = build_path_compressed(prefix, args$extract_filename);
	f$info$extracted_cutoff = F;
	mkdir(prefix);
	}

event file_extraction_limit(f: fa_file, args: Files::AnalyzerArgs, limit: count, len: count) &priority=10
	{
	f$info$extracted_cutoff = T;
	f$info$extracted_size = limit;
	}

event zeek_init() &priority=10
	{
	Files::register_analyzer_add_callback(Files::ANALYZER_EXTRACT, on_add);
	}
