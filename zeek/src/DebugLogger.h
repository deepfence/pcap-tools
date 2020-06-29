// A logger for (selective) debugging output. Only compiled in if DEBUG is
// defined.

#pragma once

#ifdef DEBUG

#include <stdio.h>
#include <string>
#include <set>

// To add a new debugging stream, add a constant here as well as
// an entry to DebugLogger::streams in DebugLogger.cc.

enum DebugStream {
	DBG_SERIAL, // Serialization
	DBG_RULES,	// Signature matching
	DBG_STRING,	// String code
	DBG_NOTIFIERS,	// Notifiers
	DBG_MAINLOOP,	// Main IOSource loop
	DBG_ANALYZER,	// Analyzer framework
	DBG_TM,		// Time-machine packet input via Brocolli
	DBG_LOGGING,	// Logging streams
	DBG_INPUT,	// Input streams
	DBG_THREADING,	// Threading system
	DBG_FILE_ANALYSIS,	// File analysis
	DBG_PLUGINS,	// Plugin system
	DBG_ZEEKYGEN,	// Zeekygen
	DBG_PKTIO,	// Packet sources and dumpers.
	DBG_BROKER,	// Broker communication
	DBG_SCRIPTS,	// Script initialization
	DBG_SUPERVISOR,	// Process supervisor

	NUM_DBGS // Has to be last
};

#define DBG_LOG(stream, args...) \
	if ( debug_logger.IsEnabled(stream) ) \
		debug_logger.Log(stream, args)
#define DBG_LOG_VERBOSE(stream, args...) \
	if ( debug_logger.IsVerbose() && debug_logger.IsEnabled(stream) ) \
		debug_logger.Log(stream, args)
#define DBG_PUSH(stream) debug_logger.PushIndent(stream)
#define DBG_POP(stream) debug_logger.PopIndent(stream)

#define PLUGIN_DBG_LOG(plugin, args...) debug_logger.Log(plugin, args)

namespace zeek::plugin { class Plugin; }

class DebugLogger {
public:
	// Output goes to stderr per default.
	DebugLogger();
	~DebugLogger();

	void OpenDebugLog(const char* filename = 0);

	void Log(DebugStream stream, const char* fmt, ...) __attribute__((format(printf, 3, 4)));
	void Log(const zeek::plugin::Plugin& plugin, const char* fmt, ...) __attribute__((format(printf, 3, 4)));

	void PushIndent(DebugStream stream)
		{ ++streams[int(stream)].indent; }
	void PopIndent(DebugStream stream)
		{ --streams[int(stream)].indent; }

	void EnableStream(DebugStream stream)
		{ streams[int(stream)].enabled = true; }
	void DisableStream(DebugStream stream)
		{ streams[int(stream)].enabled = false; }

	// Takes comma-seperated list of stream prefixes.
	void EnableStreams(const char* streams);

	bool IsEnabled(DebugStream stream) const
		{ return streams[int(stream)].enabled; }

	void SetVerbose(bool arg_verbose)	{ verbose = arg_verbose; }
	bool IsVerbose() const			{ return verbose; }

	void ShowStreamsHelp();

private:
	FILE* file;
	bool verbose;

	struct Stream {
		const char* prefix;
		int indent;
		bool enabled;
	};

	std::set<std::string> enabled_streams;

	static Stream streams[NUM_DBGS];
};

extern DebugLogger debug_logger;

#else
#define DBG_LOG(args...)
#define DBG_LOG_VERBOSE(args...)
#define DBG_PUSH(stream)
#define DBG_POP(stream)
#define PLUGIN_DBG_LOG(plugin, args...)
#endif
