// See the file "COPYING" in the main distribution directory for copyright.

#include "zeek-config.h"

#include "DFA.h"
#include "EquivClass.h"
#include "Desc.h"
#include "Hash.h"

unsigned int DFA_State::transition_counter = 0;

DFA_State::DFA_State(int arg_state_num, const EquivClass* ec,
			NFA_state_list* arg_nfa_states,
			AcceptingSet* arg_accept)
	{
	state_num = arg_state_num;
	num_sym = ec->NumClasses();
	nfa_states = arg_nfa_states;
	accept = arg_accept;
	mark = nullptr;

	SymPartition(ec);

	xtions = new DFA_State*[num_sym];

	for ( int i = 0; i < num_sym; ++i )
		xtions[i] = DFA_UNCOMPUTED_STATE_PTR;
	}

DFA_State::~DFA_State()
	{
	delete [] xtions;
	delete nfa_states;
	delete accept;
	delete meta_ec;
	}

void DFA_State::AddXtion(int sym, DFA_State* next_state)
	{
	xtions[sym] = next_state;
	}

void DFA_State::SymPartition(const EquivClass* ec)
	{
	// Partitioning is done by creating equivalence classes for those
	// characters which have out-transitions from the given state.  Thus
	// we are really creating equivalence classes of equivalence classes.
	meta_ec = new EquivClass(ec->NumClasses());

	assert(nfa_states);
	for ( int i = 0; i < nfa_states->length(); ++i )
		{
		NFA_State* n = (*nfa_states)[i];
		int sym = n->TransSym();

		if ( sym == SYM_EPSILON )
			continue;

		if ( sym != SYM_CCL )
			{ // character transition
			if ( ec->IsRep(sym) )
				{
				sym = ec->SymEquivClass(sym);
				meta_ec->UniqueChar(sym);
				}
			continue;
			}

		// Character class.
		meta_ec->CCL_Use(n->TransCCL());
		}

	meta_ec->BuildECs();
	}

DFA_State* DFA_State::ComputeXtion(int sym, DFA_Machine* machine)
	{
	int equiv_sym = meta_ec->EquivRep(sym);
	if ( xtions[equiv_sym] != DFA_UNCOMPUTED_STATE_PTR )
		{
		AddXtion(sym, xtions[equiv_sym]);
		return xtions[sym];
		}

	const EquivClass* ec = machine->EC();

	DFA_State* next_d;

	NFA_state_list* ns = SymFollowSet(equiv_sym, ec);
	if ( ns->length() > 0 )
		{
		NFA_state_list* state_set = epsilon_closure(ns);
		if ( ! machine->StateSetToDFA_State(state_set, next_d, ec) )
			delete state_set;
		}
	else
		{
		delete ns;
		next_d = nullptr;	// Jam
		}

	AddXtion(equiv_sym, next_d);
	if ( sym != equiv_sym )
		AddXtion(sym, next_d);

	return xtions[sym];
	}

void DFA_State::AppendIfNew(int sym, int_list* sym_list)
	{
	for ( auto value : *sym_list )
		if ( value == sym )
			return;

	sym_list->push_back(sym);
	}

NFA_state_list* DFA_State::SymFollowSet(int ec_sym, const EquivClass* ec)
	{
	NFA_state_list* ns = new NFA_state_list;

	assert(nfa_states);

	for ( int i = 0; i < nfa_states->length(); ++i )
		{
		NFA_State* n = (*nfa_states)[i];

		if ( n->TransSym() == SYM_CCL )
			{ // it's a character class
			CCL* ccl = n->TransCCL();
			int_list* syms = ccl->Syms();

			if ( ccl->IsNegated() )
				{
				size_t j;
				for ( j = 0; j < syms->size(); ++j )
					{
					// Loop through (sorted) negated
					// character class, which has
					// presumably already been converted
					// over to equivalence classes.
					if ( (*syms)[j] >= ec_sym )
						break;
					}

				if ( j >= syms->size() || (*syms)[j] > ec_sym )
					// Didn't find ec_sym in ccl.
					n->AddXtionsTo(ns);

				continue;
				}

			for ( auto sym : *syms )
				{
				if ( sym > ec_sym )
					break;

				if ( sym == ec_sym )
					{
					n->AddXtionsTo(ns);
					break;
					}
				}
 			}

		else if ( n->TransSym() == SYM_EPSILON )
			{ // do nothing
			}

		else if ( ec->IsRep(n->TransSym()) )
			{
			if ( ec_sym == ec->SymEquivClass(n->TransSym()) )
				n->AddXtionsTo(ns);
			}
		}

	ns->resize(0);
	return ns;
	}

void DFA_State::ClearMarks()
	{
	if ( mark )
		{
		SetMark(nullptr);

		for ( int i = 0; i < num_sym; ++i )
			{
			DFA_State* s = xtions[i];

			if ( s && s != DFA_UNCOMPUTED_STATE_PTR )
				xtions[i]->ClearMarks();
			}
		}
	}

void DFA_State::Describe(ODesc* d) const
	{
	d->Add("DFA state");
	}

void DFA_State::Dump(FILE* f, DFA_Machine* m)
	{
	if ( mark )
		return;

	fprintf(f, "\nDFA state %d:", StateNum());

	if ( accept )
		{
		AcceptingSet::const_iterator it;

		for ( it = accept->begin(); it != accept->end(); ++it )
			fprintf(f, "%s accept #%d", it == accept->begin() ? "" : ",", *it);
		}

	fprintf(f, "\n");

	int num_trans = 0;
	for ( int sym = 0; sym < num_sym; ++sym )
		{
		DFA_State* s = xtions[sym];

		if ( ! s )
			continue;

		// Look ahead for compression.
		int i;
		for ( i = sym + 1; i < num_sym; ++i )
			if ( xtions[i] != s )
				break;

		char xbuf[512];

		int r = m->Rep(sym);
		if ( ! r )
			r = '.';

		if ( i == sym + 1 )
			sprintf(xbuf, "'%c'", r);
		else
			sprintf(xbuf, "'%c'-'%c'", r, m->Rep(i-1));

		if ( s == DFA_UNCOMPUTED_STATE_PTR )
			fprintf(f, "%stransition on %s to <uncomputed>",
				++num_trans == 1 ? "\t" : "\n\t", xbuf);
		else
			fprintf(f, "%stransition on %s to state %d",
				++num_trans == 1 ? "\t" : "\n\t", xbuf,
				s->StateNum());

		sym = i - 1;
		}

	if ( num_trans > 0 )
		fprintf(f, "\n");

	SetMark(this);

	for ( int sym = 0; sym < num_sym; ++sym )
		{
		DFA_State* s = xtions[sym];

		if ( s && s != DFA_UNCOMPUTED_STATE_PTR )
			s->Dump(f, m);
		}
	}

void DFA_State::Stats(unsigned int* computed, unsigned int* uncomputed)
	{
	for ( int sym = 0; sym < num_sym; ++sym )
		{
		DFA_State* s = xtions[sym];

		if ( s == DFA_UNCOMPUTED_STATE_PTR )
			(*uncomputed)++;
		else
			(*computed)++;
		}
	}

unsigned int DFA_State::Size()
	{
	return sizeof(*this)
		+ pad_size(sizeof(DFA_State*) * num_sym)
		+ (accept ? pad_size(sizeof(int) * accept->size()) : 0)
		+ (nfa_states ? pad_size(sizeof(NFA_State*) * nfa_states->length()) : 0)
		+ (meta_ec ? meta_ec->Size() : 0);
	}

DFA_State_Cache::DFA_State_Cache()
	{
	hits = misses = 0;
	}

DFA_State_Cache::~DFA_State_Cache()
	{
	for ( auto& entry : states )
		{
		assert(entry.second);
		Unref(entry.second);
		}

	states.clear();
	}

DFA_State* DFA_State_Cache::Lookup(const NFA_state_list& nfas, DigestStr* digest)
	{
	// We assume that state ID's don't exceed 10 digits, plus
	// we allow one more character for the delimiter.
	auto id_tag_buf = std::make_unique<u_char[]>(nfas.length() * 11 + 1);
	auto id_tag = id_tag_buf.get();
	u_char* p = id_tag;

	for ( int i = 0; i < nfas.length(); ++i )
		{
		NFA_State* n = nfas[i];
		if ( n->TransSym() != SYM_EPSILON || n->Accept() != NO_ACCEPT )
			{
			int id = n->ID();
			do
				{
				*p++ = '0' + (char)(id % 10);
				id /= 10;
				}
			while ( id > 0 );
			*p++ = '&';
			}
		}

	*p++ = '\0';

	// We use the short MD5 instead of the full string for the
	// HashKey because the data is copied into the key.
	hash128_t hash;
	KeyedHash::Hash128(id_tag, p - id_tag, &hash);
	*digest = DigestStr(reinterpret_cast<const unsigned char*>(hash), 16);

	auto entry = states.find(*digest);
	if ( entry == states.end() )
		{
		++misses;
		return nullptr;
		}
	++hits;

	digest->clear();

	return entry->second;
	}

DFA_State* DFA_State_Cache::Insert(DFA_State* state, DigestStr digest)
	{
	states.emplace(std::move(digest), state);
	return state;
	}

void DFA_State_Cache::GetStats(Stats* s)
	{
	s->dfa_states = 0;
	s->nfa_states = 0;
	s->computed = 0;
	s->uncomputed = 0;
	s->mem = 0;
	s->hits = hits;
	s->misses = misses;

	for ( const auto& state : states )
		{
		DFA_State* e = state.second;
		++s->dfa_states;
		s->nfa_states += e->NFAStateNum();
		e->Stats(&s->computed, &s->uncomputed);
		s->mem += pad_size(e->Size()) + padded_sizeof(*e);
		}
	}

DFA_Machine::DFA_Machine(NFA_Machine* n, EquivClass* arg_ec)
	{
	state_count = 0;

	nfa = n;
	Ref(n);

	ec = arg_ec;

	dfa_state_cache = new DFA_State_Cache();

	NFA_state_list* ns = new NFA_state_list;
	ns->push_back(n->FirstState());

	if ( ns->length() > 0 )
		{
		NFA_state_list* state_set = epsilon_closure(ns);
		StateSetToDFA_State(state_set, start_state, ec);
		}
	else
		{
		start_state = nullptr; // Jam
		delete ns;
		}
	}

DFA_Machine::~DFA_Machine()
	{
	delete dfa_state_cache;
	Unref(nfa);
	}

void DFA_Machine::Describe(ODesc* d) const
	{
	d->Add("DFA machine");
	}

void DFA_Machine::Dump(FILE* f)
	{
	start_state->Dump(f, this);
	start_state->ClearMarks();
	}

unsigned int DFA_Machine::MemoryAllocation() const
	{
	DFA_State_Cache::Stats s;
	dfa_state_cache->GetStats(&s);

	// FIXME: Count *ec?
	return padded_sizeof(*this)
		+ s.mem
		+ padded_sizeof(*start_state)
		+ nfa->MemoryAllocation();
	}

bool DFA_Machine::StateSetToDFA_State(NFA_state_list* state_set,
				DFA_State*& d, const EquivClass* ec)
	{
	DigestStr digest;
	d = dfa_state_cache->Lookup(*state_set, &digest);

	if ( d )
		return false;

	AcceptingSet* accept = new AcceptingSet;

	for ( int i = 0; i < state_set->length(); ++i )
		{
		int acc = (*state_set)[i]->Accept();

		if ( acc != NO_ACCEPT )
			accept->insert(acc);
		}

	if ( accept->empty() )
		{
		delete accept;
		accept = nullptr;
		}

	DFA_State* ds = new DFA_State(state_count++, ec, state_set, accept);
	d = dfa_state_cache->Insert(ds, std::move(digest));

	return true;
	}

int DFA_Machine::Rep(int sym)
	{
	for ( int i = 0; i < NUM_SYM; ++i )
		if ( ec->SymEquivClass(i) == sym )
			return i;

	return -1;
	}
