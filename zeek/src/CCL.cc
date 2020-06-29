// See the file "COPYING" in the main distribution directory for copyright.

#include "zeek-config.h"

#include <algorithm>

#include "CCL.h"
#include "RE.h"
#include "DFA.h"

CCL::CCL()
	{
	syms = new int_list;
	index = -(rem->InsertCCL(this) + 1);
	negated = 0;
	}

CCL::~CCL()
	{
	delete syms;
	}

void CCL::Negate()
	{
	negated = 1;
	Add(SYM_BOL);
	Add(SYM_EOL);
	}

void CCL::Add(int sym)
	{
	ptr_compat_int sym_p = ptr_compat_int(sym);

	// Check to see if the character is already in the ccl.
	for ( auto sym : *syms )
		if ( sym == sym_p )
			return;

	syms->push_back(sym_p);
	}

void CCL::Sort()
	{
	std::sort(syms->begin(), syms->end());
	}

unsigned int CCL::MemoryAllocation() const
	{
	return padded_sizeof(*this) + padded_sizeof(*syms) + pad_size(syms->size() * sizeof(int_list::value_type));
	}
