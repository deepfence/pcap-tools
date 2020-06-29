#pragma once

#include <map>
#include <utility>
#include <list>
#include <string>

#include "util.h"
ZEEK_FORWARD_DECLARE_NAMESPACED(Stmt, zeek::detail);

/**
 * A simple class for managing stats of Bro script coverage across Bro runs.
 */
class Brofiler {
public:
	Brofiler();
	virtual ~Brofiler();

	/**
	 * Imports Bro script Stmt usage information from file pointed to by
	 * environment variable ZEEK_PROFILER_FILE.
	 *
	 * @return: true if usage info was read, otherwise false.
	 */
	bool ReadStats();

	/**
	 * Combines usage stats from current run with any read from ReadStats(),
	 * then writes information to file pointed to by environment variable
	 * ZEEK_PROFILER_FILE.  If the value of that env. variable ends with
	 * ".XXXXXX" (exactly 6 X's), then it is first passed through mkstemp
	 * to get a unique file.
	 *
	 * @return: true when usage info is written, otherwise false.
	 */
	bool WriteStats();

	void SetDelim(char d) { delim = d; }

	void IncIgnoreDepth() { ignoring++; }
	void DecIgnoreDepth() { ignoring--; }

	void AddStmt(zeek::detail::Stmt* s);

private:
	/**
	 * The current, global Brofiler instance creates this list at parse-time.
	 */
	std::list<zeek::detail::Stmt*> stmts;

	/**
	 * Indicates whether new statments will not be considered as part of
	 * coverage statistics because it was marked with the @no-test tag.
	 */
	uint32_t ignoring;

	/**
	 * The character to use to delimit Brofiler output files.  Default is '\t'.
	 */
	char delim;

	/**
	 * This maps Stmt location-desc pairs to the total number of times that
	 * Stmt has been executed.  The map can be initialized from a file at
	 * startup time and modified at shutdown time before writing back
	 * to a file.
	 */
	std::map<std::pair<std::string, std::string>, uint64_t> usage_map;

	/**
	 * A canonicalization routine for Stmt descriptions containing characters
	 * that don't agree with the output format of Brofiler.
	 */
	struct canonicalize_desc {
		char delim;

		void operator() (char& c)
			{
			if ( c == '\n' ) c = ' ';
			if ( c == delim ) c = ' ';
			}
	};
};

extern Brofiler brofiler;
