// See the file "COPYING" in the main distribution directory for copyright.

#pragma once

#include "IntrusivePtr.h"

class Val;

/**
 * A simple wrapper class to use for the return value of BIFs so that
 * they may return either a Val* or IntrusivePtr<Val> (the former could
 * potentially be deprecated).
 */
class BifReturnVal {
public:

	template <typename T>
	BifReturnVal(IntrusivePtr<T> v) noexcept
		: rval(AdoptRef{}, v.release())
		{ }

	BifReturnVal(std::nullptr_t) noexcept;

	[[deprecated("Remove in v4.1.  Return an IntrusivePtr instead.")]]
	BifReturnVal(Val* v) noexcept;

	IntrusivePtr<Val> rval;
};
