##! A module for executing external command line programs.

@load base/frameworks/input

module Exec;

export {
	type Command: record {
		## The command line to execute.  Use care to avoid injection
		## attacks (i.e., if the command uses untrusted/variable data,
		## sanitize it with :zeek:see:`safe_shell_quote`).
		cmd:         string;
		## Provide standard input to the program as a string.
		stdin:       string      &default="";
		## If additional files are required to be read in as part of the
		## output of the command they can be defined here.
		read_files:  set[string] &optional;
		## The unique id for tracking executors.
		uid: string &default=unique_id("");
	};

	type Result: record {
		## Exit code from the program.
		exit_code:    count            &default=0;
		## True if the command was terminated with a signal.
		signal_exit:  bool             &default=F;
		## Each line of standard output.
		stdout:       vector of string &optional;
		## Each line of standard error.
		stderr:       vector of string &optional;
		## If additional files were requested to be read in
		## the content of the files will be available here.
		files:        table[string] of string_vec &optional;
	};

	## Function for running command line programs and getting
	## output.  This is an asynchronous function which is meant
	## to be run with the `when` statement.
	##
	## cmd: The command to run.  Use care to avoid injection attacks!
	##
	## Returns: A record representing the full results from the
	##          external program execution.
	global run: function(cmd: Command): Result;
}

# Indexed by command uid.
global results: table[string] of Result;
global pending_commands: set[string];
global pending_files: table[string] of set[string];

type OneLine: record {
	s: string;
	is_stderr: bool;
};

type FileLine: record {
	s: string;
};

event Exec::line(description: Input::EventDescription, tpe: Input::Event, s: string, is_stderr: bool)
	{
	local result = results[description$name];
	if ( is_stderr )
		{
		if ( ! result?$stderr )
			result$stderr = vector(s);
		else
			result$stderr += s;
		}
	else
		{
		if ( ! result?$stdout )
			result$stdout = vector(s);
		else
			result$stdout += s;
		}
	}

event Exec::file_line(description: Input::EventDescription, tpe: Input::Event, s: string)
	{
	local parts = split_string1(description$name, /_/);
	local name = parts[0];
	local track_file = parts[1];

	local result = results[name];
	if ( ! result?$files )
		result$files = table();

	if ( track_file !in result$files )
		result$files[track_file] = vector(s);
	else
		result$files[track_file] += s;
	}

event Input::end_of_data(orig_name: string, source:string)
	{
	local name = orig_name;
	local parts = split_string1(name, /_/);
	name = parts[0];

	if ( name !in pending_commands || |parts| < 2 )
		return;

	local track_file = parts[1];

	# If the file is empty, still add it to the result$files table. This is needed
	# because it is expected that the file was read even if it was empty.
	local result = results[name];
	if ( ! result?$files )
		result$files = table();

	if ( track_file !in result$files )
		result$files[track_file] = vector();

	Input::remove(orig_name);

	if ( name !in pending_files )
		delete pending_commands[name];
	else
		{
		delete pending_files[name][track_file];
		if ( |pending_files[name]| == 0 )
			delete pending_commands[name];
		system(fmt("rm %s", safe_shell_quote(track_file)));
		}
	}

event InputRaw::process_finished(name: string, source:string, exit_code:count, signal_exit:bool)
	{
	if ( name !in pending_commands )
		return;

	# Upon the process exiting, the internal Raw reader code should take
	# care of signalling that the stream needs to be disabled/removed.
	#Input::remove(name);
	results[name]$exit_code = exit_code;
	results[name]$signal_exit = signal_exit;

	if ( name !in pending_files || |pending_files[name]| == 0 )
		# No extra files to read, command is done.
		delete pending_commands[name];
	else
		for ( read_file in pending_files[name] )
			Input::add_event([$source=fmt("%s", read_file),
			                  $name=fmt("%s_%s", name, read_file),
			                  $reader=Input::READER_RAW,
			                  $want_record=F,
			                  $fields=FileLine,
			                  $ev=Exec::file_line]);
	}

function run(cmd: Command): Result
	{
	add pending_commands[cmd$uid];
	results[cmd$uid] = [];

	if ( cmd?$read_files )
		{
		for ( read_file in cmd$read_files )
			{
			if ( cmd$uid !in pending_files )
				pending_files[cmd$uid] = set();
			add pending_files[cmd$uid][read_file];
			}
		}

	local config_strings: table[string] of string = {
		["stdin"]       = cmd$stdin,
		["read_stderr"] = "1",
	};
	Input::add_event([$name=cmd$uid,
	                  $source=fmt("%s |", cmd$cmd),
	                  $reader=Input::READER_RAW,
	                  $mode=Input::STREAM,
	                  $fields=Exec::OneLine,
	                  $ev=Exec::line,
	                  $want_record=F,
	                  $config=config_strings]);

	return when ( cmd$uid !in pending_commands )
		{
		local result = results[cmd$uid];
		delete results[cmd$uid];
		return result;
		}
	}

event zeek_done()
	{
	# We are punting here and just deleting any unprocessed files.
	for ( uid in pending_files )
		for ( fname in pending_files[uid] )
			system(fmt("rm %s", safe_shell_quote(fname)));
	}
