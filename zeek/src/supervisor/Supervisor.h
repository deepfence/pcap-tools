// See the file "COPYING" in the main distribution directory for copyright.

#pragma once

#include <sys/types.h>
#include <optional>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <utility>
#include <memory>
#include <chrono>
#include <map>

#include "iosource/IOSource.h"
#include "Timer.h"
#include "Pipe.h"
#include "Flare.h"
#include "NetVar.h"
#include "IntrusivePtr.h"
#include "Options.h"

namespace zeek {

/**
 * A Supervisor object manages a tree of persistent Zeek processes.  If any
 * child process dies it will be re-created with its original configuration.
 * The Supervisor process itself actually only manages a single child process,
 * called the Stem process.  That Stem is created via a fork() just after the
 * command-line arguments have been parsed.  The Stem process is used as the
 * baseline image for spawning and supervising further Zeek child nodes since
 * it has the purest global state without having to risk an exec() using an
 * on-disk binary that's changed in the meantime from the original Supervisor's
 * version of the Zeek binary.  However, if the Stem process itself dies
 * prematurely, the Supervisor will have to fork() and exec() to revive it (and
 * then the revived Stem will re-spawn its own children).  Any node in the tree
 * will self-terminate if it detects its parent has died and that detection is
 * done via polling for change in parent process ID.
 */
class Supervisor : public iosource::IOSource {
public:

	/**
	 * Configuration options that change Supervisor behavior.
	 */
	struct Config {
		/**
		 * The filesystem path of the Zeek binary/executable.  This is used
		 * if the Stem process ever dies and we need to fork() and exec() to
		 * re-create it.
		 */
		std::string zeek_exe_path;
	};

	/**
	 * Configuration options that influence how a Supervised Zeek node
	 * integrates into the normal Zeek Cluster Framework.
	 */
	struct ClusterEndpoint {
		/**
		 * The node's role within the cluster.  E.g. manager, logger, worker.
		 */
		BifEnum::Supervisor::ClusterRole role;
		/**
		 * The host/IP at which the cluster node is listening for connections.
		 */
		std::string host;
		/**
		 * The TCP port number at which the cluster node listens for connections.
		 */
		int port;
		/**
		 * The interface name from which the node read/analyze packets.
		 * Typically used by worker nodes.
		 */
		std::optional<std::string> interface;
	};

	/**
	 * Configuration options that influence behavior of a Supervised Zeek node.
	 */
	struct NodeConfig {
		/**
		 * Create configuration from script-layer record value.
		 * @param node_val  the script-layer record value to convert.
		 */
		static NodeConfig FromRecord(const RecordVal* node_val);

		/**
		 * Create configuration from JSON representation.
		 * @param json  the JSON string to convert.
		 */
		static NodeConfig FromJSON(std::string_view json);

		/**
		 * Convert this object into JSON respresentation.
		 * @return  the JSON string representing the node config.
		 */
		std::string ToJSON() const;

		/**
		 * Convert his object into script-layer record value.
		 * @return  the script-layer record value representing the node config.
		 */
		IntrusivePtr<RecordVal> ToRecord() const;

		/**
		 * The name of the supervised Zeek node.  These are unique within
		 * a given supervised process tree and typically human-readable.
		 */
		std::string name;
		/**
		 * The interface name from which the node should read/analyze packets.
		 */
		std::optional<std::string> interface;
		/**
		 * The working directory that should be used by the node.
		 */
		std::optional<std::string> directory;
		/**
		 * The filename/path to which the node's stdout will be redirected.
		 */
		std::optional<std::string> stdout_file;
		/**
		 * The filename/path to which the node's stderr will be redirected.
		 */
		std::optional<std::string> stderr_file;
		/**
		 * A cpu/core number to which the node will try to pin itself.
		 */
		std::optional<int> cpu_affinity;
		/**
		 * Additional script filename/paths that the node should load.
		 */
		std::vector<std::string> scripts;
		/**
		 * The Cluster Layout definition.  Each node in the Cluster Framework
		 * knows about the full, static cluster topology to which it belongs.
		 * Entries in the map use node names for keys.
		 */
		std::map<std::string, ClusterEndpoint> cluster;
	};

	/**
	 * State which defines a Supervised node's understanding of itself.
	 */
	struct SupervisedNode {
		/**
		 * Initialize the Supervised node within the Zeek Cluster Framework.
		 * This function populates the "Cluster::nodes" script-layer variable
		 * that otherwise is expected to be populated by a
		 * "cluster-layout.zeek" script in other context (e.g. ZeekCtl
		 * generates that cluster layout).
		 * @return  true if the supervised node is using the Cluster Framework
		 * else false.
		 */
		bool InitCluster() const;

		/**
		 * Initialize the Supervised node.
		 * @param options  the Zeek options to extend/modify as appropriate
		 * for the node's configuration.
		 */
		void Init(zeek::Options* options) const;

		/**
		 * The node's configuration options.
		 */
		NodeConfig config;
		/**
		 * The process ID of the supervised node's parent process (i.e. the PID
		 * of the Stem process).
		 */
		pid_t parent_pid;
	};

	/**
	 * The state of a supervised node from the Supervisor's perspective.
	 */
	struct Node {
		/**
		 * Convert the node into script-layer Supervisor::NodeStatus record
		 * representation.
		 */
		IntrusivePtr<RecordVal> ToRecord() const;

		/**
		 * @return the name of the node.
		 */
		const std::string& Name() const
			{ return config.name; }

		/**
		 * Create a new node state from a given configuration.
		 * @param arg_config  the configuration to use for the node.
		 */
		Node(NodeConfig arg_config) : config(std::move(arg_config))
			{ }

		/**
		 * The desired configuration for the node.
		 */
		NodeConfig config;
		/**
		 * Process ID of the node (positive/non-zero are valid/live PIDs).
		 */
		pid_t pid = 0;
		/**
		 * Whether the node is voluntarily marked for termination by the
		 * Supervisor.
		 */
		bool killed = false;
		/**
		 * The last exit status of the node.
		 */
		int exit_status = 0;
		/**
		 * The last signal which terminated the node.
		 */
		int signal_number = 0;
		/**
		 * Number of process revival attempts made after the node first died
		 * prematurely.
		 */
		int revival_attempts = 0;
		/**
		 * How many seconds to wait until the next revival attempt for the node.
		 */
		int revival_delay = 1;
		/**
		 * The time at which the node's process was last spawned.
		 */
		std::chrono::time_point<std::chrono::steady_clock> spawn_time;
	};

	/**
	 * State used to initalialize the Stem process.
	 */
	struct StemState {
		/**
		 * Bidirectional pipes that allow the Supervisor and Stem to talk.
		 */
		std::unique_ptr<zeek::detail::PipePair> pipe;
		/**
		 * The Stem's parent process ID (i.e. PID of the Supervisor).
		 */
		pid_t parent_pid = 0;
		/**
		 * The Stem's process ID.
		 */
		pid_t pid = 0;
	};

	/**
	 * Create and run the Stem process if necessary.
	 * @param supervisor_mode  whether Zeek was invoked with the supervisor
	 * mode specified as command-line argument/option.
	 * @return  state that defines the Stem process if called from the
	 * Supervisor process.  The Stem process itself will not return from this,
	 * function but a node it spawns via fork() will return from it and
	 * information about it is available in ThisNode().
	 */
	static std::optional<StemState> CreateStem(bool supervisor_mode);

	/**
	 * @return  the state which describes what a supervised node should know
	 * about itself if this is a supervised process.  If called from a process
	 * that is not supervised, this returns an "empty" object.
	 */
	static const std::optional<SupervisedNode>& ThisNode()
		{ return supervised_node; }

	using NodeMap = std::map<std::string, Node, std::less<>>;

	/**
	 * Create a new Supervisor object.
	 * @param stem_state  information about the Stem process that was already
	 * created via CreateStem()
	 */
	Supervisor(Config cfg, StemState stem_state);

	/**
	 * Destruction also cleanly shuts down the entire supervised process tree.
	 */
	~Supervisor();

	/**
	 * Perform some initialization that needs to happen after scripts are loaded
	 * and the IOSource manager is created.
	 */
	void InitPostScript();

	/**
	 * @return the process ID of the Stem.
	 */
	pid_t StemPID() const
		{ return stem_pid; }

	/**
	 * @return the state of currently supervised processes.  The map uses
	 * node names for keys.
	 */
	const NodeMap& Nodes()
		{ return nodes; }

	/**
	 * Retrieve current status of a supervised node.
	 * @param node_name  the name of the node for which to retrieve status
	 * or an empty string to mean "all nodes".
	 * @return  script-layer Supervisor::Status record value describing the
	 * status of a node or set of nodes.
	 */
	IntrusivePtr<RecordVal> Status(std::string_view node_name);

	/**
	 * Create a new supervised node.
	 * @param node  the script-layer Supervisor::NodeConfig value that
	 * describes the desired node configuration
	 * @return  an empty string on success or description of the error/failure
	 */
	std::string Create(const RecordVal* node);

	/**
	 * Create a new supervised node.
	 * @param node  the desired node configuration
	 * @return  an empty string on success or description of the error/failure
	 */
	std::string Create(const Supervisor::NodeConfig& node);

	/**
	 * Destroys and removes a supervised node.
	 * @param node_name  the name of the node to destroy or an empty string
	 * to mean "all nodes"
	 * @return  true on success
	 */
	bool Destroy(std::string_view node_name);

	/**
	 * Restart a supervised node process (by destroying and re-recreating).
	 * @param node_name  the name of the node to restart or an empty string
	 * to mean "all nodes"
	 * @return  true on success
	 */
	bool Restart(std::string_view node_name);

	/**
	 * Not meant for public use.  For use in a signal handler to tell the
	 * Supervisor a child process (i.e. the Stem) potentially died.
	 */
	void ObserveChildSignal(int signo);

private:

	// IOSource interface overrides:
	double GetNextTimeout() override;
	void Process() override;

	size_t ProcessMessages();

	void HandleChildSignal();

	void ReapStem();

	const char* Tag() override
		{ return "zeek::Supervisor"; }

	/**
	 * Run the Stem process.  The Stem process will receive instructions from
	 * the Supervisor to manipulate the process hierarchy and it's in charge
	 * of directly monitoring for whether any nodes die premature and need
	 * to be revived.
	 * @param pipe  bidirectional pipes that allow the Supervisor and Stem
	 * process to communicate.
	 * @param pid  the Stem's parent process ID (i.e. the PID of the Supervisor)
	 * @return  state which describes what a supervised node should know about
	 * itself.  I.e. this function only returns from a fork()'d child process.
	 */
	static SupervisedNode RunStem(StemState stem_state);

	static std::optional<SupervisedNode> supervised_node;

	Config config;
	pid_t stem_pid;
	std::unique_ptr<zeek::detail::PipePair> stem_pipe;
	int last_signal = -1;
	zeek::detail::Flare signal_flare;
	NodeMap nodes;
	std::string msg_buffer;
};

/**
 * A timer used by supervised processes to periodically check whether their
 * parent (supervisor) process has died.  If it has died, the supervised
 * process self-terminates.
 */
class ParentProcessCheckTimer final : public Timer {
public:

	/**
	 * Create a timer to check for parent process death.
	 * @param t  the time at which to trigger the timer's check.
	 * @param interval  number of seconds to wait before checking again.
	 */
	ParentProcessCheckTimer(double t, double interval);

protected:

	void Dispatch(double t, bool is_expire) override;

	double interval;
};

extern Supervisor* supervisor_mgr;

} // namespace zeek
