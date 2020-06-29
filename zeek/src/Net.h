// See the file "COPYING" in the main distribution directory for copyright.

#pragma once

#include <sys/stat.h> // for ino_t

#include <list>
#include <vector>
#include <string>
#include <optional>

namespace iosource {
	class IOSource;
	class PktSrc;
	class PktDumper;
	}

class Packet;

extern void net_init(const std::optional<std::string>& interfaces,
                     const std::optional<std::string>& pcap_input_file,
                     const std::optional<std::string>& pcap_output_file,
                     bool do_watchdog);
extern void net_run();
extern void net_get_final_stats();
extern void net_finish(int drain_events);
extern void net_delete();	// Reclaim all memory, etc.
extern void net_update_time(double new_network_time);
extern void net_packet_dispatch(double t, const Packet* pkt,
			iosource::PktSrc* src_ps);
extern void expire_timers(iosource::PktSrc* src_ps = nullptr);
extern void zeek_terminate_loop(const char* reason);

// Functions to temporarily suspend processing of live input (network packets
// and remote events/state). Turning this is on is sure to lead to data loss!
extern void net_suspend_processing();
extern void net_continue_processing();

extern int _processing_suspended;	// don't access directly.
inline bool net_is_processing_suspended()
	{ return _processing_suspended > 0; }

// Whether we're reading live traffic.
extern bool reading_live;

// Same but for reading from traces instead.  We have two separate
// variables because it's possible that neither is true, and we're
// instead just running timers (per the variable after this one).
extern bool reading_traces;

// True if we have timers scheduled for the future on which we need
// to wait.  "Need to wait" here means that we're running live (though
// perhaps not reading_live, but just running in real-time) as opposed
// to reading a trace (in which case we don't want to wait in real-time
// on future timers).
extern bool have_pending_timers;

// If > 0, we are reading from traces but trying to mimic real-time behavior.
// (In this case, both reading_traces and reading_live are true.)  The value
// is the speedup (1 = real-time, 0.5 = half real-time, etc.).
extern double pseudo_realtime;

// When we started processing the current packet and corresponding event
// queue.
extern double processing_start_time;

// When the Bro process was started.
extern double bro_start_time;

// Time at which the Bro process was started with respect to network time,
// i.e. the timestamp of the first packet.
extern double bro_start_network_time;

// True if we're a in the process of cleaning-up just before termination.
extern bool terminating;

// True if Bro is currently parsing scripts.
extern bool is_parsing;

extern const Packet* current_pkt;
extern int current_dispatched;
extern double current_timestamp;
extern iosource::PktSrc* current_pktsrc;
extern iosource::IOSource* current_iosrc;

extern iosource::PktDumper* pkt_dumper;	// where to save packets

// Script file we have already scanned (or are in the process of scanning).
// They are identified by device and inode number.
struct ScannedFile {
	dev_t dev;
	ino_t inode;
	int include_level;
	bool skipped;		// This ScannedFile was @unload'd.
	bool prefixes_checked;	// If loading prefixes for this file has been tried.
	std::string name;

	ScannedFile(dev_t arg_dev, ino_t arg_inode, int arg_include_level,
	            const std::string& arg_name, bool arg_skipped = false,
	            bool arg_prefixes_checked = false)
		: dev(arg_dev), inode(arg_inode),
		  include_level(arg_include_level),
		  skipped(arg_skipped),
		  prefixes_checked(arg_prefixes_checked),
		  name(arg_name)
		{ }
};

extern std::list<ScannedFile> files_scanned;
extern std::vector<std::string> sig_files;
