// See the file "COPYING" in the main distribution directory for copyright.

#pragma once

#include <memory>

#include "Type.h"
#include "IntrusivePtr.h"

class ListVal;
class HashKey;

class CompositeHash {
public:
	explicit CompositeHash(IntrusivePtr<zeek::TypeList> composite_type);
	~CompositeHash();

	// Compute the hash corresponding to the given index val,
	// or nullptr if it fails to typecheck.
	std::unique_ptr<HashKey> MakeHashKey(const Val& v, bool type_check) const;

	[[deprecated("Remove in v4.1.  Use MakeHashKey().")]]
	HashKey* ComputeHash(const Val* v, bool type_check) const
		{ return MakeHashKey(*v, type_check).release(); }

	// Given a hash key, recover the values used to create it.
	IntrusivePtr<ListVal> RecoverVals(const HashKey& k) const;

	[[deprecated("Remove in v4.1.  Pass in HashKey& instead.")]]
	IntrusivePtr<ListVal> RecoverVals(const HashKey* k) const
		{ return RecoverVals(*k); }

	unsigned int MemoryAllocation() const { return padded_sizeof(*this) + pad_size(size); }

protected:
	std::unique_ptr<HashKey> ComputeSingletonHash(const Val* v, bool type_check) const;

	// Computes the piece of the hash for Val*, returning the new kp.
	// Used as a helper for ComputeHash in the non-singleton case.
	char* SingleValHash(bool type_check, char* kp, zeek::Type* bt, Val* v,
			    bool optional) const;

	// Recovers just one Val of possibly many; called from RecoverVals.
	// Upon return, pval will point to the recovered Val of type t.
	// Returns and updated kp for the next Val.  Calls reporter->InternalError()
	// upon errors, so there is no return value for invalid input.
	const char* RecoverOneVal(const HashKey& k,
				  const char* kp, const char* const k_end,
				  zeek::Type* t, IntrusivePtr<Val>* pval, bool optional) const;

	// Rounds the given pointer up to the nearest multiple of the
	// given size, if not already a multiple.
	const void* Align(const char* ptr, unsigned int size) const;

	// Rounds the given pointer up to the nearest multiple of the
	// given size, padding the skipped region with 0 bytes.
	void* AlignAndPad(char* ptr, unsigned int size) const;

	// Returns offset+size rounded up so it can correctly align data
	// of the given size.
	int SizeAlign(int offset, unsigned int size) const;

	template<class T>
	T* AlignAndPadType(char* ptr) const
		{
		return reinterpret_cast<T*>(AlignAndPad(ptr, sizeof(T)));
		}

	template<class T>
	const T* AlignType(const char* ptr) const
		{
		return reinterpret_cast<const T*>(Align(ptr, sizeof(T)));
		}

	template<class T>
	int SizeAlignType(int offset) const
		{
		return SizeAlign(offset, sizeof(T));
		}

	// Compute the size of the composite key.  If v is non-nil then
	// the value is computed for the particular list of values.
	// Returns 0 if the key has an indeterminant size (if v not given),
	// or if v doesn't match the index type (if given).
	int ComputeKeySize(const Val* v, bool type_check,
			   bool calc_static_size) const;

	int SingleTypeKeySize(zeek::Type*, const Val*,
			      bool type_check, int sz, bool optional,
			      bool calc_static_size) const;

	IntrusivePtr<zeek::TypeList> type;
	char* key;	// space for composite key
	int size;
	bool is_singleton;	// if just one type in index

	// If one type, but not normal "singleton", e.g. record.
	bool is_complex_type;

	zeek::InternalTypeTag singleton_tag;
};
