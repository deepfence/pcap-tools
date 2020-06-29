// See the file "COPYING" in the main distribution directory for copyright.

#pragma once

#include <sys/types.h> // for u_char

#include <list>
#include <vector>
#include <tuple>
#include <type_traits>

#include "Tag.h"

#include "../Obj.h"
#include "../EventHandler.h"
#include "../Timer.h"
#include "../IntrusivePtr.h"

class BroFile;
class Rule;
class Connection;
class IP_Hdr;

namespace analyzer {

namespace tcp { class TCP_ApplicationAnalyzer; }
namespace pia { class PIA; }

class Analyzer;
class AnalyzerTimer;
class SupportAnalyzer;
class OutputHandler;

using analyzer_list = std::list<Analyzer*>;
typedef uint32_t ID;
typedef void (Analyzer::*analyzer_timer_func)(double t);

/**
 * Class to receive processed output from an anlyzer.
 */
class OutputHandler {
public:
	/**
	 * Destructor.
	 */
	virtual	~OutputHandler() { }

	/**
	 * Hook for receiving packet data. Parameters are the same as for
	 * Analyzer::DeliverPacket().
	 */
	virtual void DeliverPacket(int len, const u_char* data,
				   bool orig, uint64_t seq,
				   const IP_Hdr* ip, int caplen)
		{ }

	/**
	 * Hook for receiving stream data. Parameters are the same as for
	 * Analyzer::DeliverStream().
	 */
	virtual void DeliverStream(int len, const u_char* data,
				   bool orig)	{ }

	/**
	 * Hook for receiving notification of stream gaps. Parameters are the
	 * same as for Analyzer::Undelivered().
	 */
	virtual void Undelivered(uint64_t seq, int len, bool orig)	{ }
};

/**
 * Main analyzer interface.
 *
 * Each analyzer is part of a tree, having a parent analyzer and an arbitrary
 * number of child analyzers. Each analyzer also has a list of
 * SupportAnalyzer. All analyzer input first passes through this list of
 * support analyzers, which can perform arbitrary preprocessing.
 *
 * When overiding any of the class' methods, always make sure to call the
 * base-class version first.
 */
class Analyzer {
public:
	/**
	 * Constructor.
	 *
	 * @param name The name for the type of analyzer. The name must match
	 * the one the corresponding Component registers.
	 *
	 * @param conn The connection the analyzer is associated with.
	 */
	Analyzer(const char* name, Connection* conn);

	/**
	 * Constructor.
	 *
	 * @param tag The tag for the type of analyzer. The tag must map to
	 * the name the corresponding Component registers.
	 *
	 * @param conn The connection the analyzer is associated with.
	 */
	Analyzer(const Tag& tag, Connection* conn);

	/**
	 * Constructor. As this version of the constructor does not receive a
	 * name or tag, SetAnalyzerTag() must be called before the instance
	 * can be used.
	 *
	 * @param conn The connection the analyzer is associated with.
	 */
	explicit Analyzer(Connection* conn);

	/**
	 * Destructor.
	 */
	virtual ~Analyzer();

	/**
	 * Initializes the analyzer before input processing starts.
	 */
	virtual void Init();

	/**
	 * Finishes the analyzer's operation after all input has been parsed.
	 */
	virtual void Done();

	/**
	 * Passes packet input to the analyzer for processing. The analyzer
	 * will process the input with any support analyzers first and then
	 * forward the data to DeliverStream(), which derived classes can
	 * override.
	 *
	 * Note that there is a separate method for stream input,
	 * NextStream().
	 *
	 * @param len The number of bytes passed in.
	 *
	 * @param data Pointer the input to process.
	 *
	 * @param is_orig True if this is originator-side input.
	 *
	 * @param seq Current sequence number, if available (only supported
	 * if the data is coming from the TCP analyzer.
	 *
	 * @param ip An IP packet header associated with the data, if
	 * available.
	 *
	 * @param caplen The packet's capture length, if available.
	 */
	void NextPacket(int len, const u_char* data, bool is_orig,
			uint64_t seq = -1, const IP_Hdr* ip = nullptr, int caplen = 0);

	/**
	 * Passes stream input to the analyzer for processing. The analyzer
	 * will process the input with any support analyzers first and then
	 * forward the data to DeliverStream(), which derived classes can
	 * override.
	 *
	 * Note that there is a separate method for packet input,
	 * NextPacket().
	 *
	 * @param len The number of bytes passed in.
	 *
	 * @param data Pointer the input to process.
	 *
	 * @param is_orig True if this is originator-side input.
	 */
	void NextStream(int len, const u_char* data, bool is_orig);

	/**
	 * Informs the analyzer about a gap in the TCP stream, i.e., data
	 * that can't be delivered. This method triggers Undelivered(), which
	 * derived classes can override.
	 *
	 * @param seq The sequence number of the first byte of gap.
	 *
	 * @param len The length of the gap.
	 *
	 * @param is_orig True if this is about originator-side input.
	 */
	void NextUndelivered(uint64_t seq, int len, bool is_orig);

	/**
	 * Reports a message boundary.  This is a generic method that can be
	 * used by an Analyzer if all data of a PDU has been delivered, e.g.,
	 * to report that HTTP body has been delivered completely by the HTTP
	 * analyzer before it starts with the next body. A final EndOfData()
	 * is automatically generated by the analyzer's Done() method. This
	 * method triggers EndOfData(), which derived classes can override.
	 *
	 * @param is_orig True if this is about originator-side input.
	 */
	void NextEndOfData(bool is_orig);

	/**
	 * Forwards packet input on to all child analyzers. If the analyzer
	 * has an associated OutputHandlers, that one receives the input as
	 * well.
	 *
	 * Parameters are the same as for NextPacket().
	 */
	virtual void ForwardPacket(int len, const u_char* data,
					bool orig, uint64_t seq,
					const IP_Hdr* ip, int caplen);

	/**
	 * Forwards stream input on to all child analyzers. If the analyzer
	 * has an associated OutputHandlers, that one receives the input as
	 * well.
	 *
	 * Parameters are the same as for NextStream().
	 */
	virtual void ForwardStream(int len, const u_char* data, bool orig);

	/**
	 * Forwards a sequence gap on to all child analyzers.
	 *
	 * Parameters are the same as for NextUndelivered().
	 */
	virtual void ForwardUndelivered(uint64_t seq, int len, bool orig);

	/**
	 * Forwards an end-of-data notification on to all child analyzers.
	 *
	 * Parameters are the same as for NextPacket().
	 */
	virtual void ForwardEndOfData(bool orig);

	/**
	 * Hook for accessing packet input for parsing. This is called by
	 * NextDeliverPacket() and can be overridden by derived classes.
	 * Parameters are the same.
	 */
	virtual void DeliverPacket(int len, const u_char* data, bool orig,
					uint64_t seq, const IP_Hdr* ip, int caplen);

	/**
	 * Hook for accessing stream input for parsing. This is called by
	 * NextDeliverStream() and can be overridden by derived classes.
	 * Parameters are the same.
	 */
	virtual void DeliverStream(int len, const u_char* data, bool orig);

	/**
	 * Hook for accessing input gap during parsing. This is called by
	 * NextUndelivered() and can be overridden by derived classes.
	 * Parameters are the same.
	 */
	virtual void Undelivered(uint64_t seq, int len, bool orig);

	/**
	 * Hook for accessing end-of-data notifications. This is called by
	 * NextEndOfData() and can be overridden by derived classes.
	 * Parameters are the same.
	 */
	virtual void EndOfData(bool is_orig);

	/**
	 * Signals the analyzer that its associated connection had its
	 * endpoint flipped. This can happen if during analysis it turns out
	 * that we got the direction of the connection wrong.  In these
	 * cases, this method is called to swap state if necessary.  This
	 * will not happen after payload has already been passed on, so most
	 * analyzers don't need to care.
	 */
	virtual void FlipRoles();

	/**
	 * Returns the analyzer instance's internal ID. These IDs are unique
	 * across all analyzer instantiated and can thus be used to indentify
	 * a specific instance.
	 */
	ID GetID() const	{ return id; }

	/**
	 * Returns the connection that the analyzer is associated with.
	 */
	Connection* Conn() const	{ return conn; }

	/**
	 * Returns the OutputHandler associated with the connection, or null
	 * if none.
	 */
	OutputHandler* GetOutputHandler() const	{ return output_handler; }

	/**
	 * Associates an OutputHandler with the connnection.
	 *
	 * @param handler The handler.
	 */
	void SetOutputHandler(OutputHandler* handler)
		{ output_handler = handler; }

	/**
	 * If this analyzer was activated by a signature match, this returns
	 * the signature that did so. Returns null otherwise.
	 */
	const Rule* Signature() const		{ return signature; }

	/**
	 * Sets the signature that activated this analyzer, if any.
	 *
	 * @param sig The signature.
	 */
	void SetSignature(const Rule* sig)	{ signature = sig; }

	/**
	 * Signals the analyzer to skip all further input processsing. The \a
	 * Next*() methods check this flag and discard the input if its set.
	 *
	 * @param do_skip If true, further processing will be skipped.
	 */
	void SetSkip(bool do_skip)		{ skip = do_skip; }

	/**
	 * Returns true if the analyzer has been told to skip processing all
	 * further input.
	 */
	bool Skipping() const			{ return skip; }

	/**
	 * Returns true if Done() has been called.
	 */
	bool IsFinished() const 		{ return finished; }

	/**
	 * Returns true if the analyzer has been flagged for removal and
	 * shouldn't be used anymore.
	 */
	bool Removing() const	{ return removing; }

	/**
	 * Returns the tag associated with the analyzer's type.
	 */
	Tag GetAnalyzerTag() const		{ assert(tag); return tag; }

	/**
	 * Sets the tag associated with the analyzer's type. Note that this
	 * can be called only right after construction, if the constructor
	 * did not receive a name or tag. The method cannot be used to change
	 * an existing tag.
	 */
	void SetAnalyzerTag(const Tag& tag);

	/**
	 * Returns a textual description of the analyzer's type. This is
	 * what's passed to the constructor and usally corresponds to the
	 * protocol name, e.g., "HTTP".
	 */
	const char* GetAnalyzerName() const;

	/**
	 * Returns true if this analyzer's type matches the name passes in.
	 * This is shortcut for comparing GetAnalyzerName() with the given
	 * name.
	 *
	 * @param name The name to check.
	 */
	bool IsAnalyzer(const char* name);

	/**
	 * Adds a new child analyzer to the analyzer tree. If an analyzer of
	 * the same type already exists or is prevented, the one passed in is
	 * silently discarded.
	 *
	 * @param analyzer The ananlyzer to add. Takes ownership.
	 * @return false if analyzer type was already a child or prevented, else true.
	 */
	bool AddChildAnalyzer(Analyzer* analyzer)
		{ return AddChildAnalyzer(analyzer, true); }

	/**
	 * Adds a new child analyzer to the analyzer tree. If an analyzer of
	 * the same type already exists or is prevented, the one passed in is
	 * silently discarded.
	 *
	 * @param tag The type of analyzer to add.
	 * @return the new analyzer instance that was added.
	 */
	Analyzer* AddChildAnalyzer(const Tag& tag);

	/**
	 * Removes a child analyzer. It's ok for the analyzer to not to be a
	 * child, in which case the method does nothing.
	 *
	 * @param analyzer The analyzer to remove.
	 *
	 * @return whether the child analyzer is scheduled for removal
	 * (and was not before).
	 */
	bool RemoveChildAnalyzer(Analyzer* analyzer)
		{ return RemoveChildAnalyzer(analyzer->GetID()); }

	/**
	 * Removes a child analyzer. It's ok for the analyzer to not to be a
	 * child, in which case the method does nothing.
	 *
	 * @param id The type of analyzer to remove.
	 *
	 * @return whether the child analyzer is scheduled for removal
	 * (and was not before).
	 */
	virtual bool RemoveChildAnalyzer(ID id);

	/**
	 * Prevents an analyzer type from ever being added as a child.
	 *
	 * @param tag The type of analyzer to prevent.
	 */
	void PreventChildren(Tag tag);

	/**
	 * Returns true if analyzer has a direct child of a given type.
	 *
	 * @param tag The type of analyzer to check for.
	 */
	bool HasChildAnalyzer(Tag tag);

	/**
	 * Recursively searches all (direct or indirect) childs of the
	 * analyzer for an analyzer with a specific ID.
	 *
	 * @param id The analyzer id to search. This is the ID that GetID()
	 * returns.
	 *
	 * @return The analyzer, or null if not found.
	 */
	virtual Analyzer* FindChild(ID id);

	/**
	 * Recursively searches all (direct or indirect) childs of the
	 * analyzer for an analyzer of a given type.
	 *
	 * @param tag The analyzer type to search.
	 *
	 * @return The first analyzer of the given type found, or null if
	 * none.
	 */
	virtual Analyzer* FindChild(Tag tag);

	/**
	 * Recursively searches all (direct or indirect) childs of the
	 * analyzer for an analyzer of a given type.
	 *
	 * @param name The naem of the analyzer type to search (e.g.,
	 * "HTTP").
	 *
	 * @return The first analyzer of the given type found, or null if
	 * none.
	 */
	Analyzer* FindChild(const char* name);

	/**
	 * Returns a list of all direct child analyzers.
	 *
	 * Note that this does not include the list of analyzers that are
	 * currently queued up to be added. If you just added an analyzer,
	 * it will not immediately be in this list.
	 */
	const analyzer_list& GetChildren()	{ return children; }

	/**
	 * Returns a pointer to the parent analyzer, or null if this instance
	 * has not yet been added to an analyzer tree.
	 */
	Analyzer* Parent() const	{ return parent; }

	/**
	 * Sets the parent analyzer.
	 *
	 * @param p The new parent.
	 */
	void SetParent(Analyzer* p)	{ parent = p; }

	/**
	 * Remove the analyzer form its parent. The analyzer must have a
	 * parent associated with it.
	 *
	 * @return whether the analyzer is being removed
	 */
	bool Remove();

	/**
	 * Appends a support analyzer to the current list.
	 *
	 * @param analyzer The support analyzer to add.
	 */
	void AddSupportAnalyzer(SupportAnalyzer* analyzer);

	/**
	 * Remove a support analyzer.
	 *
	 * @param analyzer The analyzer to remove. The function is a no-op if
	 * that analyzer is not part of the list of support analyzer.
	 */
	void RemoveSupportAnalyzer(SupportAnalyzer* analyzer);

	/**
	 * Signals Bro's protocol detection that the analyzer has recognized
	 * the input to indeed conform to the expected protocol. This should
	 * be called as early as possible during a connection's life-time. It
	 * may turn into \c protocol_confirmed event at the script-layer (but
	 * only once per analyzer for each connection, even if the method is
	 * called multiple times).
	 *
	 * If tag is given, it overrides the analyzer tag passed to the
	 * scripting layer; the default is the one of the analyzer itself.
	 */
	virtual void ProtocolConfirmation(Tag tag = Tag());

	/**
	 * Signals Bro's protocol detection that the analyzer has found a
	 * severe protocol violation that could indicate that it's not
	 * parsing the expected protocol. This turns into \c
	 * protocol_violation events at the script-layer (one such event is
	 * raised for each call to this method so that the script-layer can
	 * built up a notion of how prevalent protocol violations are; the
	 * more, the less likely it's the right protocol).
	 *
	 * @param reason A textual description of the error encountered.
	 *
	 * @param data An optional pointer to the malformed data.
	 *
	 * @param len If \a data is given, the length of it.
	 */
	virtual void ProtocolViolation(const char* reason,
					const char* data = nullptr, int len = 0);

	/**
	 * Returns true if ProtocolConfirmation() has been called at least
	 * once.
	 */
	bool ProtocolConfirmed() const
		{ return protocol_confirmed; }

	/**
	 * Called whenever the connection value is updated. Per default, this
	 * method will be called for each analyzer in the tree. Analyzers can
	 * use this method to attach additional data to the connections. A
	 * call to BuildConnVal() will in turn trigger a call to
	 * UpdateConnVal().
	 *
	 * @param conn_val The connenction value being updated.
	 */
	virtual void UpdateConnVal(RecordVal *conn_val);

	/**
	 * Convenience function that forwards directly to
	 * Connection::BuildConnVal().
	 */
	[[deprecated("Remove in v4.1.  Use ConnVal() instead.")]]
	RecordVal* BuildConnVal();

	/**
	 * Convenience function that forwards directly to
	 * Connection::ConnVal().
	 */
	const IntrusivePtr<RecordVal>& ConnVal();

	/**
	 * Convenience function that forwards directly to the corresponding
	 * Connection::Event().
	 */
	void Event(EventHandlerPtr f, const char* name = nullptr);

	/**
	 * Convenience function that forwards directly to the corresponding
	 * Connection::Event().
	 */
	[[deprecated("Remove in v4.1.  Use EnqueueConnEvent() instead (note it doesn't automatically ad the connection argument).")]]
	void Event(EventHandlerPtr f, Val* v1, Val* v2 = nullptr);

	/**
	 * Convenience function that forwards directly to
	 * Connection::ConnectionEvent().
	 */
	[[deprecated("Remove in v4.1.  Use EnqueueConnEvent() instead.")]]
	void ConnectionEvent(EventHandlerPtr f, val_list* vl);

	/**
	 * Convenience function that forwards directly to
	 * Connection::ConnectionEvent().
	 */
	[[deprecated("Remove in v4.1.  Use EnqueueConnEvent() instead.")]]
	void ConnectionEvent(EventHandlerPtr f, val_list vl);

	/**
	 * Convenience function that forwards directly to
	 * Connection::ConnectionEventFast().
	 */
	[[deprecated("Remove in v4.1.  Use EnqueueConnEvent() instead.")]]
	void ConnectionEventFast(EventHandlerPtr f, val_list vl);

	/**
	 * Convenience function that forwards directly to
	 * Connection::EnqueueEvent().
	 */
	void EnqueueConnEvent(EventHandlerPtr f, zeek::Args args);

	/**
	 * A version of EnqueueConnEvent() taking a variable number of arguments.
	 */
	template <class... Args>
	std::enable_if_t<
	  std::is_convertible_v<
	    std::tuple_element_t<0, std::tuple<Args...>>, IntrusivePtr<Val>>>
	EnqueueConnEvent(EventHandlerPtr h, Args&&... args)
		{ return EnqueueConnEvent(h, zeek::Args{std::forward<Args>(args)...}); }

	/**
	 * Convenience function that forwards directly to the corresponding
	 * Connection::Weird().
	 */
	void Weird(const char* name, const char* addl = "");

	/**
	 * Internal method.
	 */
	virtual unsigned int MemoryAllocation() const;

protected:
	friend class AnalyzerTimer;
	friend class Manager;
	friend class ::Connection;
	friend class tcp::TCP_ApplicationAnalyzer;

	/**
	 * Return a string represantation of an analyzer, containing its name
	 * and ID.
	 */
	static std::string fmt_analyzer(const Analyzer* a)
		{ return std::string(a->GetAnalyzerName()) + fmt("[%d]", a->GetID()); }

	/**
	 * Associates a connection with this analyzer.  Must be called if
	 * using the default ctor.
	 *
	 * @param c The connection.
	 */
	void SetConnection(Connection* c)	{ conn = c; }

	/**
	 * Instantiates a new timer associated with the analyzer.
	 *
	 * @param timer The callback function to execute when the timer
	 *  fires.
	 *
	 * @param  t The absolute time when the timer will fire.
	 *
	 * @param do_expire If true, the timer will also fire when Bro
	 * terminates even if \a t has not been reache yet.
	 *
	 * @param type The timer's type.
	 */
	void AddTimer(analyzer_timer_func timer, double t, bool do_expire,
			TimerType type);

	/**
	 * Cancels all timers added previously via AddTimer().
	 */
	void CancelTimers();

	/**
	 * Removes a given timer. This is an internal method and shouldn't be
	 * used by derived class. It does not cancel the timer.
	 */
	void RemoveTimer(Timer* t);

	/**
	 * Returns true if the analyzer has associated an SupportAnalyzer of a given type.
	 *
	 * @param tag The type to check for.
	 *
	 * @param orig True if asking about the originator side.
	 */
	bool HasSupportAnalyzer(const Tag& tag, bool orig);

	/**
	 * Returns the first still active support analyzer for the given
	 * direction, or null if none.
	 *
	 * @param orig True if asking about the originator side.
	 */
	SupportAnalyzer* FirstSupportAnalyzer(bool orig);

	/**
	 * Adds a a new child analyzer with the option whether to intialize
	 * it. This is an internal method.
	 *
	 * @param analyzer The analyzer to add. Takes ownership.
	 *
	 * @param init If true, Init() will be calle.d
	 * @return false if analyzer type was already a child, else true.
	 */
	bool AddChildAnalyzer(Analyzer* analyzer, bool init);

	/**
	 * Inits all child analyzers. This is an internal method.
	 */
	void InitChildren();

	/**
	 * Reorganizes the child data structure. This is an internal method.
	 */
	void AppendNewChildren();

	/**
	 * Returns true if the child analyzer is now scheduled to be
	 * removed (and was not before)
	 */
	bool RemoveChild(const analyzer_list& children, ID id);

private:
	// Internal method to eventually delete a child analyzer that's
	// already Done().
	void DeleteChild(analyzer_list::iterator i);

	// Helper for the ctors.
	void CtorInit(const Tag& tag, Connection* conn);

	Tag tag;
	ID id;

	Connection* conn;
	Analyzer* parent;
	const Rule* signature;
	OutputHandler* output_handler;

	analyzer_list children;
	SupportAnalyzer* orig_supporters;
	SupportAnalyzer* resp_supporters;

	analyzer_list new_children;
	std::vector<Tag> prevented;

	bool protocol_confirmed;

	timer_list timers;
	bool timers_canceled;
	bool skip;
	bool finished;
	bool removing;

	static ID id_counter;
};

/**
 * Convenience macro to add a new timer.
 */
#define ADD_ANALYZER_TIMER(timer, t, do_expire, type) \
	   AddTimer(analyzer::analyzer_timer_func(timer), (t), (do_expire), (type))

/**
 * Internal convenience macro to iterate over the list of child analyzers.
 */
#define LOOP_OVER_CHILDREN(var) \
	for ( analyzer::analyzer_list::iterator var = children.begin(); \
	      var != children.end(); var++ )

/**
 * Internal convenience macro to iterate over the constant list of child
 * analyzers.
 */
#define LOOP_OVER_CONST_CHILDREN(var) \
	for ( analyzer::analyzer_list::const_iterator var = children.begin(); \
	      var != children.end(); var++ )

/**
 * Convenience macro to iterate over a given list of child analyzers.
 */
#define LOOP_OVER_GIVEN_CHILDREN(var, the_kids) \
	for ( analyzer::analyzer_list::iterator var = the_kids.begin(); \
	      var != the_kids.end(); var++ )

/**
 * Convenience macro to iterate over a given constant list of child
 * analyzers.
 */
#define LOOP_OVER_GIVEN_CONST_CHILDREN(var, the_kids) \
	for ( analyzer::analyzer_list::const_iterator var = the_kids.begin(); \
	      var != the_kids.end(); var++ )

/**
 * Support analyzer preprocess input before it reaches an analyzer's main
 * processing. They share the input interface with of an Analyzer but they
 * are uni-directional: they receive data only from one side of a connection.
 *
 */
class SupportAnalyzer : public Analyzer {
public:
	/**
	 * Constructor.
	 *
	 * @param name A name for the protocol the analyzer is parsing. The
	 * name must match the one the corresponding Component registers.
	 *
	 * @param conn The connection the analyzer is associated with.
	 *
	 * @param arg_orig: If true, this is a support analyzer for the
	 * connection originator side, and otherwise for the responder side.
	 */
	SupportAnalyzer(const char* name, Connection* conn, bool arg_orig)
		: Analyzer(name, conn)	{ orig = arg_orig; sibling = nullptr; }

	/**
	 * Destructor.
	 */
	~SupportAnalyzer() override {}

	/**
	 * Returns true if this is a support analyzer for the connection's
	 * originator side.
	 */
	bool IsOrig() const 	{ return orig; }

	/**
	 * Returns the analyzer's next sibling, or null if none.
	 *
	 * only_active: If true, this will skip siblings that are still link
	 * but flagged for removal.
	 */
	SupportAnalyzer* Sibling(bool only_active = false) const;

	/**
	* Passes packet input to the next sibling SupportAnalyzer if any, or
	* on to the associated main analyzer if none. If however there's an
	* output handler associated with this support analyzer, the data is
	* passed only to there.
	*
	* Parameters same as for Analyzer::ForwardPacket.
	*/
	void ForwardPacket(int len, const u_char* data, bool orig,
					uint64_t seq, const IP_Hdr* ip, int caplen) override;

	/**
	* Passes stream input to the next sibling SupportAnalyzer if any, or
	* on to the associated main analyzer if none. If however there's an
	* output handler associated with this support analyzer, the data is
	* passed only to there.
	*
	* Parameters same as for Analyzer::ForwardStream.
	*/
	void ForwardStream(int len, const u_char* data, bool orig) override;

	/**
	* Passes gap information to the next sibling SupportAnalyzer if any,
	* or on to the associated main analyzer if none. If however there's
	* an output handler associated with this support analyzer, the gap is
	* passed only to there.
	*
	* Parameters same as for Analyzer::ForwardPacket.
	*/
	void ForwardUndelivered(uint64_t seq, int len, bool orig) override;

protected:
	friend class Analyzer;

private:
	bool orig;

	// Points to next support analyzer in chain.  The list is managed by
	// parent analyzer.
	SupportAnalyzer* sibling;
};

// The following need to be consistent with bro.init.
#define CONTENTS_NONE 0
#define CONTENTS_ORIG 1
#define CONTENTS_RESP 2
#define CONTENTS_BOTH 3

/**
 * Base class for analyzers parsing transport-layer protocols.
 */
class TransportLayerAnalyzer : public Analyzer {
public:
	/**
	 * Constructor.
	 *
	 * @param name A name for the protocol the analyzer is parsing. The
	 * name must match the one the corresponding Component registers.
	 *
	 * @param conn The connection the analyzer is associated with.
	 */
	TransportLayerAnalyzer(const char* name, Connection* conn)
		: Analyzer(name, conn)	{ pia = nullptr; }

	/**
	 * Overridden from parent class.
	 */
	void Done() override;

	/**
	 * Returns true if the analyzer determines that in fact a new
	 * connection has started without the connection statement having
	 * terminated the previous one, i.e., the new data is arriving at
	 * what's the analyzer for the previous instance. This is used only
	 * for TCP.
	 */
	virtual bool IsReuse(double t, const u_char* pkt) = 0;

	/**
	 * Associates a file with the analyzer in which to record all
	 * analyzed input. This must only be called with derived classes that
	 * overide the method; the default implementation will abort.
	 *
	 * @param direction One of the CONTENTS_* constants indicating which
	 * direction of the input stream is to be recorded.
	 *
	 * @param f The file to record to.
	 *
	 */
	virtual void SetContentsFile(unsigned int direction, IntrusivePtr<BroFile> f);

	/**
	 * Returns an associated contents file, if any.  This must only be
	 * called with derived classes that overide the method; the default
	 * implementation will abort.
	 *
	 * @param direction One of the CONTENTS_* constants indicating which
	 * direction the query is for.
	 */
	virtual IntrusivePtr<BroFile> GetContentsFile(unsigned int direction) const;

	/**
	 * Associates a PIA with this analyzer. A PIA takes the
	 * transport-layer input and determine which protocol analyzer(s) to
	 * use for parsing it.
	 */
	void SetPIA(pia::PIA* arg_PIA)	{ pia = arg_PIA; }

	/**
	 * Returns the associated PIA, or null of none. Does not take
	 * ownership.
	 */
	pia::PIA* GetPIA() const		{ return pia; }

	/**
	 * Helper to raise a \c packet_contents event.
	 *
	 * @param data The dass to pass to the event.
	 *
	 * @param len The length of \a data.
	 */
	void PacketContents(const u_char* data, int len);

private:
	pia::PIA* pia;
};

}
