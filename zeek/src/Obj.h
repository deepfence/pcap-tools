// See the file "COPYING" in the main distribution directory for copyright.

#pragma once

#include <limits.h>

class ODesc;

class Location final {
public:
	constexpr Location(const char* fname, int line_f, int line_l,
			   int col_f, int col_l) noexcept
		:filename(fname), first_line(line_f), last_line(line_l),
		 first_column(col_f), last_column(col_l) {}

	Location() = default;

	void Describe(ODesc* d) const;

	bool operator==(const Location& l) const;
	bool operator!=(const Location& l) const
		{ return ! (*this == l); }

	const char* filename = nullptr;
	int first_line = 0, last_line = 0;
	int first_column = 0, last_column = 0;
};

#define YYLTYPE yyltype
typedef Location yyltype;
YYLTYPE GetCurrentLocation();

// Used to mean "no location associated with this object".
inline constexpr Location no_location("<no location>", 0, 0, 0, 0);

// Current start/end location.
extern Location start_location;
extern Location end_location;

// Used by parser to set the above.
inline void set_location(const Location loc)
	{
	start_location = end_location = loc;
	}

inline void set_location(const Location start, const Location end)
	{
	start_location = start;
	end_location = end;
	}

class BroObj {
public:
	BroObj()
		{
		// A bit of a hack.  We'd like to associate location
		// information with every object created when parsing,
		// since for them, the location is generally well-defined.
		// We could maintain a separate flag that tells us whether
		// we're inside a parse, but the parser also sets the
		// location to no_location when it's done, so it makes
		// sense to just check for that.  *However*, start_location
		// and end_location are maintained as their own objects
		// rather than pointers or references, so we can't directly
		// check them for equality with no_location.  So instead
		// we check for whether start_location has a line number
		// of 0, which should only happen if it's been assigned
		// to no_location (or hasn't been initialized at all).
		location = nullptr;
		if ( start_location.first_line != 0 )
			SetLocationInfo(&start_location, &end_location);
		}

	virtual ~BroObj();

	/* disallow copying */
	BroObj(const BroObj &) = delete;
	BroObj &operator=(const BroObj &) = delete;

	// Report user warnings/errors.  If obj2 is given, then it's
	// included in the message, though if pinpoint_only is non-zero,
	// then obj2 is only used to pinpoint the location.
	void Warn(const char* msg, const BroObj* obj2 = nullptr,
			bool pinpoint_only = false, const Location* expr_location = nullptr) const;
	void Error(const char* msg, const BroObj* obj2 = nullptr,
			bool pinpoint_only = false, const Location* expr_location = nullptr) const;

	// Report internal errors.
	void BadTag(const char* msg, const char* t1 = nullptr,
			const char* t2 = nullptr) const;
#define CHECK_TAG(t1, t2, text, tag_to_text_func) \
	{ \
	if ( t1 != t2 ) \
		BadTag(text, tag_to_text_func(t1), tag_to_text_func(t2)); \
	}

	[[noreturn]] void Internal(const char* msg) const;
	void InternalWarning(const char* msg) const;

	virtual void Describe(ODesc* d) const { /* FIXME: Add code */ };

	void AddLocation(ODesc* d) const;

	// Get location info for debugging.
	const Location* GetLocationInfo() const
		{ return location ? location : &no_location; }

	virtual bool SetLocationInfo(const Location* loc)
		{ return SetLocationInfo(loc, loc); }

	// Location = range from start to end.
	virtual bool SetLocationInfo(const Location* start, const Location* end);

	// Set new end-of-location information.  This is used to
	// extend compound objects such as statement lists.
	virtual void UpdateLocationEndInfo(const Location& end);

	// Enable notification of plugins when this objects gets destroyed.
	void NotifyPluginsOnDtor()	{ notify_plugins = true; }

	int RefCnt() const	{ return ref_cnt; }

	// Helper class to temporarily suppress errors
	// as long as there exist any instances.
	class SuppressErrors {
	public:
		SuppressErrors()	{ ++BroObj::suppress_errors; }
		~SuppressErrors()	{ --BroObj::suppress_errors; }
	};

protected:
	Location* location;	// all that matters in real estate

private:
	friend class SuppressErrors;

	void DoMsg(ODesc* d, const char s1[], const BroObj* obj2 = nullptr,
			bool pinpoint_only = false, const Location* expr_location = nullptr) const;
	void PinPoint(ODesc* d, const BroObj* obj2 = nullptr,
			bool pinpoint_only = false) const;

	friend inline void Ref(BroObj* o);
	friend inline void Unref(BroObj* o);

	bool notify_plugins = false;
	int ref_cnt = 1;

	// If non-zero, do not print runtime errors.  Useful for
	// speculative evaluation.
	static int suppress_errors;
};

// Prints obj to stderr, primarily for debugging.
extern void print(const BroObj* obj);

[[noreturn]] extern void bad_ref(int type);

// Sometimes useful when dealing with BroObj subclasses that have their
// own (protected) versions of Error.
inline void Error(const BroObj* o, const char* msg)
	{
	o->Error(msg);
	}

inline void Ref(BroObj* o)
	{
	if ( ++(o->ref_cnt) <= 1 )
		bad_ref(0);
	if ( o->ref_cnt == INT_MAX )
		bad_ref(1);
	}

inline void Unref(BroObj* o)
	{
	if ( o && --o->ref_cnt <= 0 )
		{
		if ( o->ref_cnt < 0 )
			bad_ref(2);
		delete o;

		// We could do the following if o were passed by reference.
		// o = (BroObj*) 0xcd;
		}
	}

// A dict_delete_func that knows to Unref() dictionary entries.
extern void bro_obj_delete_func(void* v);
