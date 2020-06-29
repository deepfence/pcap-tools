// See the file "COPYING" in the main distribution directory for copyright.

#pragma once

#include <vector>
#include <string>
#include <iosfwd>

#include <sys/types.h>

typedef u_char* byte_vec;

// Forward declaration, for helper functions that convert (sub)string vectors
// to and from policy-level representations.
//
class VectorVal;

class BroString {
public:
	typedef std::vector<BroString*> Vec;
	typedef Vec::iterator VecIt;
	typedef Vec::const_iterator VecCIt;

	typedef std::vector<const BroString*> CVec;
	typedef Vec::iterator CVecIt;
	typedef Vec::const_iterator CVecCIt;

	// IdxVecs are vectors of indices of characters in a string.
	typedef std::vector<int> IdxVec;
	typedef IdxVec::iterator IdxVecIt;
	typedef IdxVec::const_iterator IdxVecCIt;

	// Constructors creating internal copies of the data passed in.
	BroString(const u_char* str, int arg_n, bool add_NUL);
	explicit BroString(const char* str);
	explicit BroString(const std::string& str);
	BroString(const BroString& bs);

	// Constructor that takes owernship of the vector passed in.
	BroString(bool arg_final_NUL, byte_vec str, int arg_n);

	BroString();
	~BroString()	{ Reset(); }

	const BroString& operator=(const BroString& bs);
	bool operator==(const BroString& bs) const;
	bool operator<(const BroString& bs) const;

	byte_vec Bytes() const	{ return b; }
	int Len() const	{ return n; }

	// Releases the string's current contents, if any, and
	// adopts the byte vector of given length.  The string will
	// manage the memory occupied by the string afterwards.
	//
	void Adopt(byte_vec bytes, int len);

	// Various flavors of methods that release the string's
	// current contents, if any, and then set the string's
	// contents to a copy of the string given by the arguments.
	//
	void Set(const u_char* str, int len, bool add_NUL=true);
	void Set(const char* str);
	void Set(const std::string& str);
	void Set(const BroString &str);

	void SetUseFreeToDelete(int use_it)
		{ use_free_to_delete = use_it; }

	const char* CheckString() const;

	enum render_style {
		ESC_NONE = 0,
		ESC_ESC  = (1 << 1),	// '\' -> "\\"
		ESC_QUOT = (1 << 2),	// '"' -> "\"", ''' -> "\'"
		ESC_HEX  = (1 << 3),	// Not in [32, 126]? -> "\xXX"
		ESC_DOT  = (1 << 4),	// Not in [32, 126]? -> "."

		// For serialization: '<string len> <string>'
		ESC_SER  = (1 << 7),
	};

	static const int EXPANDED_STRING =	// the original style
		ESC_HEX;

	static const int BRO_STRING_LITERAL =	// as in a Bro string literal
		ESC_ESC | ESC_QUOT | ESC_HEX;

	// Renders a string into a newly allocated character array that
	// you have to delete[].  You can combine the render styles given
	// above to achieve the representation you desire.  If you pass a
	// pointer to an integer as the final argument, you'll receive the
	// entire length of the resulting char* in it.
	//
	// Note that you need to delete[] the resulting string.
	//
	char* Render(int format = EXPANDED_STRING, int* len = nullptr) const;

	// Similar to the above, but useful for output streams.
	// Also more useful for debugging purposes since no deallocation
	// is required on your part here.
	//
	std::ostream& Render(std::ostream& os, int format = ESC_SER) const;

	// Reads a string from an input stream.  Unless you use a render
	// style combination that uses ESC_SER, note that the streams
	// will consider whitespace as a field delimiter.
	//
	std::istream& Read(std::istream& is, int format = ESC_SER);

	// XXX Fix redundancy: strings.bif implements both to_lower
	// XXX and to_upper; the latter doesn't use BroString::ToUpper().
	void ToUpper();

	unsigned int MemoryAllocation() const;

	// Returns new string containing the substring of this string,
	// starting at @start >= 0 for going up to @length elements,
	// A negative @length means "until end of string".  Other invalid
	// values result in a return value of 0.
	//
	BroString* GetSubstring(int start, int length) const;

	// Returns the start index of s in this string, counting from 0.
	// If s is not found, -1 is returned.
	//
	int FindSubstring(const BroString* s) const;

	// Splits the string into substrings, taking all the indices in
	// the given vector as cutting points.  The vector does not need
	// to be sorted, and can have multiple entries.  Out-of-bounds
	// indices are ignored.  All returned strings are newly allocated.
	//
	Vec* Split(const IdxVec& indices) const;

	// Helper functions for vectors:
	static VectorVal* VecToPolicy(Vec* vec);
	static Vec* VecFromPolicy(VectorVal* vec);
	static char* VecToString(const Vec* vec);

protected:
	void Reset();

	byte_vec b;
	int n;
	bool final_NUL;	// whether we have added a final NUL
	bool use_free_to_delete;	// free() vs. operator delete
};

// A comparison class that sorts pointers to BroString's according to
// the length of the pointed-to strings. Sort order can be specified
// through the constructor.
//
class BroStringLenCmp {
public:
	explicit BroStringLenCmp(bool increasing = true) { _increasing = increasing; }
	bool operator()(BroString*const& bst1, BroString*const& bst2);

 private:
	unsigned int _increasing;
};

// Default output stream operator, using rendering mode EXPANDED_STRING.
std::ostream& operator<<(std::ostream& os, const BroString& bs);

extern int Bstr_eq(const BroString* s1, const BroString* s2);
extern int Bstr_cmp(const BroString* s1, const BroString* s2);

// A data_chunk_t specifies a length-delimited constant string. It is
// often used for substrings of other BroString's to avoid memory copy,
// which would be necessary if BroString were used. Unlike BroString,
// the string should not be deallocated on destruction.
//
// "BroConstString" might be a better name here.

struct data_chunk_t {
	int length;
	const char* data;
};

extern BroString* concatenate(std::vector<data_chunk_t>& v);
extern BroString* concatenate(BroString::Vec& v);
extern BroString* concatenate(BroString::CVec& v);
extern void delete_strings(std::vector<const BroString*>& v);
