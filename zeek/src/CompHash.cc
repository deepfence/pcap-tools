// See the file "COPYING" in the main distribution directory for copyright.

#include "zeek-config.h"

#include <cstring>
#include <vector>
#include <map>

#include "CompHash.h"
#include "BroString.h"
#include "Dict.h"
#include "Val.h"
#include "RE.h"
#include "Reporter.h"
#include "Func.h"
#include "IPAddr.h"

CompositeHash::CompositeHash(IntrusivePtr<zeek::TypeList> composite_type)
	: type(std::move(composite_type))
	{
	singleton_tag = zeek::TYPE_INTERNAL_ERROR;

	// If the only element is a record, don't treat it as a
	// singleton, since it needs to be evaluated specially.

	if ( type->Types().size() == 1 )
		{
		if ( type->Types()[0]->Tag() == zeek::TYPE_RECORD )
			{
			is_complex_type = true;
			is_singleton = false;
			}
		else
			{
			is_complex_type = false;
			is_singleton = true;
			}
		}

	else
		{
		is_singleton = false;
		is_complex_type = false;
		}

	if ( is_singleton )
		{
		// Don't do any further key computations - we'll do them
		// via the singleton later.
		singleton_tag = type->Types()[0]->InternalType();
		size = 0;
		key = nullptr;
		}

	else
		{
		size = ComputeKeySize(nullptr, true, true);

		if ( size > 0 )
			// Fixed size.  Make sure what we get is fully aligned.
			key = reinterpret_cast<char*>
				(new double[size/sizeof(double) + 1]);
		else
			key = nullptr;
		}
	}

CompositeHash::~CompositeHash()
	{
	delete [] key;
	}

// Computes the piece of the hash for Val*, returning the new kp.
char* CompositeHash::SingleValHash(bool type_check, char* kp0,
				   zeek::Type* bt, Val* v, bool optional) const
	{
	char* kp1 = nullptr;
	zeek::InternalTypeTag t = bt->InternalType();

	if ( optional )
		{
		// Add a marker saying whether the optional field is set.
		char* kp = AlignAndPadType<char>(kp0);
		*kp = ( v ? 1 : 0);
		kp0 = reinterpret_cast<char*>(kp+1);

		if ( ! v )
			return kp0;
		}

	if ( type_check )
		{
		zeek::InternalTypeTag vt = v->GetType()->InternalType();
		if ( vt != t )
			return nullptr;
		}

	switch ( t ) {
	case zeek::TYPE_INTERNAL_INT:
		{
		bro_int_t* kp = AlignAndPadType<bro_int_t>(kp0);
		*kp = v->ForceAsInt();
		kp1 = reinterpret_cast<char*>(kp+1);
		}
		break;

	case zeek::TYPE_INTERNAL_UNSIGNED:
		{
		bro_uint_t* kp = AlignAndPadType<bro_uint_t>(kp0);
		*kp = v->ForceAsUInt();
		kp1 = reinterpret_cast<char*>(kp+1);
		}
		break;

	case zeek::TYPE_INTERNAL_ADDR:
		{
		uint32_t* kp = AlignAndPadType<uint32_t>(kp0);
		v->AsAddr().CopyIPv6(kp);
		kp1 = reinterpret_cast<char*>(kp+4);
		}
		break;

	case zeek::TYPE_INTERNAL_SUBNET:
		{
		uint32_t* kp = AlignAndPadType<uint32_t>(kp0);
		v->AsSubNet().Prefix().CopyIPv6(kp);
		kp[4] = v->AsSubNet().Length();
		kp1 = reinterpret_cast<char*>(kp+5);
		}
		break;

	case zeek::TYPE_INTERNAL_DOUBLE:
		{
		double* kp = AlignAndPadType<double>(kp0);
		*kp = v->InternalDouble();
		kp1 = reinterpret_cast<char*>(kp+1);
		}
		break;

	case zeek::TYPE_INTERNAL_VOID:
	case zeek::TYPE_INTERNAL_OTHER:
		{
		switch ( v->GetType()->Tag() ) {
		case zeek::TYPE_FUNC:
			{
			uint32_t* kp = AlignAndPadType<uint32_t>(kp0);
			*kp = v->AsFunc()->GetUniqueFuncID();
			kp1 = reinterpret_cast<char*>(kp+1);
			break;
			}

		case zeek::TYPE_PATTERN:
			{
			const char* texts[2] = {
				v->AsPattern()->PatternText(),
				v->AsPattern()->AnywherePatternText()
			};

			uint64_t* kp;
			for ( int i = 0; i < 2; i++ )
				{
				kp = AlignAndPadType<uint64_t>(kp0+i);
				*kp = strlen(texts[i]) + 1;
				}

			kp1 = reinterpret_cast<char*>(kp+1);
			for ( int i = 0; i < 2; i++ )
				{
				memcpy(kp1, texts[i], strlen(texts[i]) + 1);
				kp1 += strlen(texts[i]) + 1;
				}

			break;
			}

		case zeek::TYPE_RECORD:
			{
			char* kp = kp0;
			RecordVal* rv = v->AsRecordVal();
			zeek::RecordType* rt = bt->AsRecordType();
			int num_fields = rt->NumFields();

			for ( int i = 0; i < num_fields; ++i )
				{
				auto rv_i = rv->GetField(i).get();

				zeek::detail::Attributes* a = rt->FieldDecl(i)->attrs.get();
				bool optional = (a && a->Find(zeek::detail::ATTR_OPTIONAL));

				if ( ! (rv_i || optional) )
					return nullptr;

				if ( ! (kp = SingleValHash(type_check, kp,
							   rt->GetFieldType(i).get(),
							   rv_i, optional)) )
					return nullptr;
				}

			kp1 = kp;
			break;
			}

		case zeek::TYPE_TABLE:
			{
			int* kp = AlignAndPadType<int>(kp0);
			TableVal* tv = v->AsTableVal();
			*kp = tv->Size();
			kp1 = reinterpret_cast<char*>(kp+1);

			auto tbl = tv->AsTable();
			auto it = tbl->InitForIteration();
			auto lv = make_intrusive<ListVal>(zeek::TYPE_ANY);

			struct HashKeyComparer {
				bool operator()(const HashKey* a, const HashKey* b) const
					{
					if ( a->Hash() != b->Hash() )
						return a->Hash() < b->Hash();
					if ( a->Size() != b->Size() )
						return a->Size() < b->Size();
					return strncmp(static_cast<const char*>(a->Key()),
					               static_cast<const char*>(b->Key()),
					               a->Size()) < 0;
					}
			};

			std::map<HashKey*, int, HashKeyComparer> hashkeys;
			HashKey* k;
			auto idx = 0;

			while ( tbl->NextEntry(k, it) )
				{
				hashkeys[k] = idx++;
				lv->Append(tv->RecreateIndex(*k));
				}

			for ( auto& kv : hashkeys )
				delete kv.first;

			for ( auto& kv : hashkeys )
				{
				auto idx = kv.second;
				const auto& key = lv->Idx(idx);

				if ( ! (kp1 = SingleValHash(type_check, kp1, key->GetType().get(),
				                            key.get(), false)) )
					return nullptr;

				if ( ! v->GetType()->IsSet() )
					{
					auto val = tv->FindOrDefault(key);

					if ( ! (kp1 = SingleValHash(type_check, kp1, val->GetType().get(),
								    val.get(), false)) )
						return nullptr;
					}
				}

			}
			break;

		case zeek::TYPE_VECTOR:
			{
			unsigned int* kp = AlignAndPadType<unsigned int>(kp0);
			VectorVal* vv = v->AsVectorVal();
			zeek::VectorType* vt = v->GetType()->AsVectorType();
			*kp = vv->Size();
			kp1 = reinterpret_cast<char*>(kp+1);
			for ( unsigned int i = 0; i < vv->Size(); ++i )
				{
				const auto& val = vv->At(i);
				unsigned int* kp = AlignAndPadType<unsigned int>(kp1);
				*kp = i;
				kp1 = reinterpret_cast<char*>(kp+1);
				kp = AlignAndPadType<unsigned int>(kp1);
				*kp = val ? 1 : 0;
				kp1 = reinterpret_cast<char*>(kp+1);

				if ( val )
					{
					if ( ! (kp1 = SingleValHash(type_check, kp1,
					                            vt->Yield().get(), val.get(),
					                            false)) )
						return nullptr;
					}
				}
			}
			break;

		case zeek::TYPE_LIST:
			{
			int* kp = AlignAndPadType<int>(kp0);
			ListVal* lv = v->AsListVal();
			*kp = lv->Length();
			kp1 = reinterpret_cast<char*>(kp+1);
			for ( int i = 0; i < lv->Length(); ++i )
				{
				Val* v = lv->Idx(i).get();
				if ( ! (kp1 = SingleValHash(type_check, kp1, v->GetType().get(), v,
				                            false)) )
					return nullptr;
				}
			}
			break;

		default:
			{
			reporter->InternalError("bad index type in CompositeHash::SingleValHash");
			return nullptr;
			}
		}

		break; // case zeek::TYPE_INTERNAL_VOID/OTHER
		}

	case zeek::TYPE_INTERNAL_STRING:
		{
		// Align to int for the length field.
		int* kp = AlignAndPadType<int>(kp0);
		const BroString* sval = v->AsString();

		*kp = sval->Len();	// so we can recover the value

		kp1 = reinterpret_cast<char*>(kp+1);

		memcpy(kp1, sval->Bytes(), sval->Len());
		kp1 += sval->Len();
		}
		break;

	case zeek::TYPE_INTERNAL_ERROR:
		return nullptr;
	}

	return kp1;
	}


std::unique_ptr<HashKey> CompositeHash::MakeHashKey(const Val& argv, bool type_check) const
	{
	auto v = &argv;

	if ( is_singleton )
		return ComputeSingletonHash(v, type_check);

	if ( is_complex_type && v->GetType()->Tag() != zeek::TYPE_LIST )
		{
		ListVal lv(zeek::TYPE_ANY);

		// Cast away const to use ListVal - but since we
		// re-introduce const on the recursive call, it should
		// be okay; the only thing is that the ListVal unref's it.
		Val* ncv = (Val*) v;
		lv.Append({NewRef{}, ncv});
		return MakeHashKey(lv, type_check);
		}

	char* k = key;

	if ( ! k )
		{
		int sz = ComputeKeySize(v, type_check, false);
		if ( sz == 0 )
			return nullptr;

		k = reinterpret_cast<char*>(new double[sz/sizeof(double) + 1]);
		type_check = false;	// no need to type-check again.
		}

	const auto& tl = type->Types();

	if ( type_check && v->GetType()->Tag() != zeek::TYPE_LIST )
		return nullptr;

	auto lv = v->AsListVal();

	if ( type_check && lv->Length() != static_cast<int>(tl.size()) )
		return nullptr;

	char* kp = k;
	for ( auto i = 0u; i < tl.size(); ++i )
		{
		kp = SingleValHash(type_check, kp, tl[i].get(), lv->Idx(i).get(), false);
		if ( ! kp )
			return nullptr;
		}

	return std::make_unique<HashKey>((k == key), (void*) k, kp - k);
	}

std::unique_ptr<HashKey> CompositeHash::ComputeSingletonHash(const Val* v, bool type_check) const
	{
	if ( v->GetType()->Tag() == zeek::TYPE_LIST )
		{
		auto lv = v->AsListVal();

		if ( type_check && lv->Length() != 1 )
			return nullptr;

		v = lv->Idx(0).get();
		}

	if ( type_check && v->GetType()->InternalType() != singleton_tag )
		return nullptr;

	switch ( singleton_tag ) {
	case zeek::TYPE_INTERNAL_INT:
	case zeek::TYPE_INTERNAL_UNSIGNED:
		return std::make_unique<HashKey>(v->ForceAsInt());

	case zeek::TYPE_INTERNAL_ADDR:
		return v->AsAddr().MakeHashKey();

	case zeek::TYPE_INTERNAL_SUBNET:
		return v->AsSubNet().MakeHashKey();

	case zeek::TYPE_INTERNAL_DOUBLE:
		return std::make_unique<HashKey>(v->InternalDouble());

	case zeek::TYPE_INTERNAL_VOID:
	case zeek::TYPE_INTERNAL_OTHER:
		if ( v->GetType()->Tag() == zeek::TYPE_FUNC )
			return std::make_unique<HashKey>(v->AsFunc()->GetUniqueFuncID());

		if ( v->GetType()->Tag() == zeek::TYPE_PATTERN )
			{
			const char* texts[2] = {
				v->AsPattern()->PatternText(),
				v->AsPattern()->AnywherePatternText()
			};
			int n = strlen(texts[0]) + strlen(texts[1]) + 2; // 2 for null
			char* key = new char[n];
			std::memcpy(key, texts[0], strlen(texts[0]) + 1);
			std::memcpy(key + strlen(texts[0]) + 1, texts[1], strlen(texts[1]) + 1);
			return std::make_unique<HashKey>(false, key, n);
			}

		reporter->InternalError("bad index type in CompositeHash::ComputeSingletonHash");
		return nullptr;

	case zeek::TYPE_INTERNAL_STRING:
		return std::make_unique<HashKey>(v->AsString());

	case zeek::TYPE_INTERNAL_ERROR:
		return nullptr;

	default:
		reporter->InternalError("bad internal type in CompositeHash::ComputeSingletonHash");
		return nullptr;
	}
	}

int CompositeHash::SingleTypeKeySize(zeek::Type* bt, const Val* v,
				     bool type_check, int sz, bool optional,
				     bool calc_static_size) const
	{
	zeek::InternalTypeTag t = bt->InternalType();

	if ( optional )
		sz = SizeAlign(sz, sizeof(char));

	if ( type_check && v )
		{
		zeek::InternalTypeTag vt = v->GetType()->InternalType();
		if ( vt != t )
			return 0;
		}

	switch ( t ) {
	case zeek::TYPE_INTERNAL_INT:
	case zeek::TYPE_INTERNAL_UNSIGNED:
		sz = SizeAlign(sz, sizeof(bro_int_t));
		break;

	case zeek::TYPE_INTERNAL_ADDR:
		sz = SizeAlign(sz, sizeof(uint32_t));
		sz += sizeof(uint32_t) * 3;	// to make a total of 4 words
		break;

	case zeek::TYPE_INTERNAL_SUBNET:
		sz = SizeAlign(sz, sizeof(uint32_t));
		sz += sizeof(uint32_t) * 4;	// to make a total of 5 words
		break;

	case zeek::TYPE_INTERNAL_DOUBLE:
		sz = SizeAlign(sz, sizeof(double));
		break;

	case zeek::TYPE_INTERNAL_VOID:
	case zeek::TYPE_INTERNAL_OTHER:
		{
		switch ( bt->Tag() ) {
		case zeek::TYPE_FUNC:
			{
			sz = SizeAlign(sz, sizeof(uint32_t));
			break;
			}

		case zeek::TYPE_PATTERN:
			{
			if ( ! v )
				return (optional && ! calc_static_size) ? sz : 0;

			sz = SizeAlign(sz, 2 * sizeof(uint64_t));
			sz += strlen(v->AsPattern()->PatternText())
				+ strlen(v->AsPattern()->AnywherePatternText()) + 2; // 2 for null terminators
			break;
			}

		case zeek::TYPE_RECORD:
			{
			const RecordVal* rv = v ? v->AsRecordVal() : nullptr;
			zeek::RecordType* rt = bt->AsRecordType();
			int num_fields = rt->NumFields();

			for ( int i = 0; i < num_fields; ++i )
				{
				zeek::detail::Attributes* a = rt->FieldDecl(i)->attrs.get();
				bool optional = (a && a->Find(zeek::detail::ATTR_OPTIONAL));

				sz = SingleTypeKeySize(rt->GetFieldType(i).get(),
						       rv ? rv->GetField(i).get() : nullptr,
						       type_check, sz, optional,
						       calc_static_size);
				if ( ! sz )
					return 0;
				}

			break;
			}

		case zeek::TYPE_TABLE:
			{
			if ( ! v )
				return (optional && ! calc_static_size) ? sz : 0;

			sz = SizeAlign(sz, sizeof(int));
			TableVal* tv = const_cast<TableVal*>(v->AsTableVal());
			auto lv = tv->ToListVal();
			for ( int i = 0; i < tv->Size(); ++i )
				{
				const auto& key = lv->Idx(i);
				sz = SingleTypeKeySize(key->GetType().get(), key.get(), type_check, sz, false,
				                       calc_static_size);
				if ( ! sz )
					return 0;

				if ( ! bt->IsSet() )
					{
					auto val = tv->FindOrDefault(key);
					sz = SingleTypeKeySize(val->GetType().get(), val.get(), type_check, sz,
					                       false, calc_static_size);
					if ( ! sz )
						return 0;
					}
				}

			break;
			}

		case zeek::TYPE_VECTOR:
			{
			if ( ! v )
				return (optional && ! calc_static_size) ? sz : 0;

			sz = SizeAlign(sz, sizeof(unsigned int));
			VectorVal* vv = const_cast<VectorVal*>(v->AsVectorVal());
			for ( unsigned int i = 0; i < vv->Size(); ++i )
				{
				const auto& val = vv->At(i);
				sz = SizeAlign(sz, sizeof(unsigned int));
				sz = SizeAlign(sz, sizeof(unsigned int));
				if ( val )
					sz = SingleTypeKeySize(bt->AsVectorType()->Yield().get(),
					                       val.get(), type_check, sz, false,
					                       calc_static_size);
				if ( ! sz ) return 0;
				}

			break;
			}

		case zeek::TYPE_LIST:
			{
			if ( ! v )
				return (optional && ! calc_static_size) ? sz : 0;

			sz = SizeAlign(sz, sizeof(int));
			ListVal* lv = const_cast<ListVal*>(v->AsListVal());
			for ( int i = 0; i < lv->Length(); ++i )
				{
				sz = SingleTypeKeySize(lv->Idx(i)->GetType().get(), lv->Idx(i).get(),
				                       type_check, sz, false, calc_static_size);
				if ( ! sz) return 0;
				}

			break;
			}

		default:
			{
			reporter->InternalError("bad index type in CompositeHash::CompositeHash");
			return 0;
			}
		}

		break; // case zeek::TYPE_INTERNAL_VOID/OTHER
		}

	case zeek::TYPE_INTERNAL_STRING:
		if ( ! v )
			return (optional && ! calc_static_size) ? sz : 0;

		// Factor in length field.
		sz = SizeAlign(sz, sizeof(int));
		sz += v->AsString()->Len();
		break;

	case zeek::TYPE_INTERNAL_ERROR:
		return 0;
	}

	return sz;
	}

int CompositeHash::ComputeKeySize(const Val* v, bool type_check, bool calc_static_size) const
	{
	const auto& tl = type->Types();

	if ( v )
		{
		if ( type_check && v->GetType()->Tag() != zeek::TYPE_LIST )
			return 0;

		auto lv = v->AsListVal();

		if ( type_check && lv->Length() != static_cast<int>(tl.size()) )
			return 0;
		}

	int sz = 0;
	for ( auto i = 0u; i < tl.size(); ++i )
		{
		sz = SingleTypeKeySize(tl[i].get(), v ? v->AsListVal()->Idx(i).get() : nullptr,
				       type_check, sz, false, calc_static_size);
		if ( ! sz )
			return 0;
		}

	return sz;
	}

namespace
	{
	inline bool is_power_of_2(bro_uint_t x)
		{
		return ((x - 1) & x) == 0;
		}
	}

const void* CompositeHash::Align(const char* ptr, unsigned int size) const
	{
	if ( ! size )
		return ptr;

	ASSERT(is_power_of_2(size));

	unsigned int mask = size - 1;	// Assume size is a power of 2.
	unsigned long l_ptr = reinterpret_cast<unsigned long>(ptr);
	unsigned long offset = l_ptr & mask;

	if ( offset > 0 )
		return reinterpret_cast<const void*>(ptr - offset + size);
	else
		return reinterpret_cast<const void*>(ptr);
	}

void* CompositeHash::AlignAndPad(char* ptr, unsigned int size) const
	{
	if ( ! size )
		return ptr;

	ASSERT(is_power_of_2(size));

	unsigned int mask = size - 1;	// Assume size is a power of 2.
	while ( (reinterpret_cast<unsigned long>(ptr) & mask) != 0 )
		// Not aligned - zero pad.
		*ptr++ = '\0';

	return reinterpret_cast<void *>(ptr);
	}

int CompositeHash::SizeAlign(int offset, unsigned int size) const
	{
	if ( ! size )
		return offset;

	ASSERT(is_power_of_2(size));

	unsigned int mask = size - 1;	// Assume size is a power of 2.
	if ( offset & mask )
		{
		offset &= ~mask;	// Round down.
		offset += size;		// Round up.
		}

	offset += size;		// Add in size.

	return offset;
	}

IntrusivePtr<ListVal> CompositeHash::RecoverVals(const HashKey& k) const
	{
	auto l = make_intrusive<ListVal>(zeek::TYPE_ANY);
	const auto& tl = type->Types();
	const char* kp = (const char*) k.Key();
	const char* const k_end = kp + k.Size();

	for ( const auto& type : tl )
		{
		IntrusivePtr<Val> v;
		kp = RecoverOneVal(k, kp, k_end, type.get(), &v, false);
		ASSERT(v);
		l->Append(std::move(v));
		}

	if ( kp != k_end )
		reporter->InternalError("under-ran key in CompositeHash::DescribeKey %zd", k_end - kp);

	return l;
	}

const char* CompositeHash::RecoverOneVal(const HashKey& k, const char* kp0,
					 const char* const k_end, zeek::Type* t,
					 IntrusivePtr<Val>* pval, bool optional) const
	{
	// k->Size() == 0 for a single empty string.
	if ( kp0 >= k_end && k.Size() > 0 )
		reporter->InternalError("over-ran key in CompositeHash::RecoverVals");

	zeek::TypeTag tag = t->Tag();
	zeek::InternalTypeTag it = t->InternalType();
	const char* kp1 = nullptr;

	if ( optional )
		{
		const char* kp = AlignType<char>(kp0);
		kp0 = kp1 = reinterpret_cast<const char*>(kp+1);

		if ( ! *kp )
			{
			*pval = nullptr;
			return kp0;
			}
		}

	switch ( it ) {
	case zeek::TYPE_INTERNAL_INT:
		{
		const bro_int_t* const kp = AlignType<bro_int_t>(kp0);
		kp1 = reinterpret_cast<const char*>(kp+1);

		if ( tag == zeek::TYPE_ENUM )
			*pval = t->AsEnumType()->GetVal(*kp);
		else if ( tag == zeek::TYPE_BOOL )
			*pval = val_mgr->Bool(*kp);
		else if ( tag == zeek::TYPE_INT )
			*pval = val_mgr->Int(*kp);
		else
			{
			reporter->InternalError("bad internal unsigned int in CompositeHash::RecoverOneVal()");
			*pval = nullptr;
			}
		}
		break;

	case zeek::TYPE_INTERNAL_UNSIGNED:
		{
		const bro_uint_t* const kp = AlignType<bro_uint_t>(kp0);
		kp1 = reinterpret_cast<const char*>(kp+1);

		switch ( tag ) {
		case zeek::TYPE_COUNT:
		case zeek::TYPE_COUNTER:
			*pval = val_mgr->Count(*kp);
			break;

		case zeek::TYPE_PORT:
			*pval = val_mgr->Port(*kp);
			break;

		default:
			reporter->InternalError("bad internal unsigned int in CompositeHash::RecoverOneVal()");
			*pval = nullptr;
			break;
		}
		}
		break;

	case zeek::TYPE_INTERNAL_DOUBLE:
		{
		const double* const kp = AlignType<double>(kp0);
		kp1 = reinterpret_cast<const char*>(kp+1);

		if ( tag == zeek::TYPE_INTERVAL )
			*pval = make_intrusive<IntervalVal>(*kp, 1.0);
		else if ( tag == zeek::TYPE_TIME )
			*pval = make_intrusive<TimeVal>(*kp);
		else
			*pval = make_intrusive<DoubleVal>(*kp);
		}
		break;

	case zeek::TYPE_INTERNAL_ADDR:
		{
		const uint32_t* const kp = AlignType<uint32_t>(kp0);
		kp1 = reinterpret_cast<const char*>(kp+4);

		IPAddr addr(IPv6, kp, IPAddr::Network);

		switch ( tag ) {
		case zeek::TYPE_ADDR:
			*pval = make_intrusive<AddrVal>(addr);
			break;

		default:
			reporter->InternalError("bad internal address in CompositeHash::RecoverOneVal()");
			*pval = nullptr;
			break;
		}
		}
		break;

	case zeek::TYPE_INTERNAL_SUBNET:
		{
		const uint32_t* const kp = AlignType<uint32_t>(kp0);
		kp1 = reinterpret_cast<const char*>(kp+5);
		*pval = make_intrusive<SubNetVal>(kp, kp[4]);
		}
		break;

	case zeek::TYPE_INTERNAL_VOID:
	case zeek::TYPE_INTERNAL_OTHER:
		{
		switch ( t->Tag() ) {
		case zeek::TYPE_FUNC:
			{
			const uint32_t* const kp = AlignType<uint32_t>(kp0);
			kp1 = reinterpret_cast<const char*>(kp+1);

			const auto& f = Func::GetFuncPtrByID(*kp);

			if ( ! f )
				reporter->InternalError("failed to look up unique function id %" PRIu32 " in CompositeHash::RecoverOneVal()", *kp);

			*pval = make_intrusive<Val>(f);
			const auto& pvt = (*pval)->GetType();

			if ( ! pvt )
				reporter->InternalError("bad aggregate Val in CompositeHash::RecoverOneVal()");

			else if ( t->Tag() != zeek::TYPE_FUNC && ! same_type(pvt, t) )
				// ### Maybe fix later, but may be fundamentally
				// un-checkable --US
				reporter->InternalError("inconsistent aggregate Val in CompositeHash::RecoverOneVal()");

			// ### A crude approximation for now.
			else if ( t->Tag() == zeek::TYPE_FUNC && pvt->Tag() != zeek::TYPE_FUNC )
				reporter->InternalError("inconsistent aggregate Val in CompositeHash::RecoverOneVal()");
			}
			break;

		case zeek::TYPE_PATTERN:
			{
			RE_Matcher* re = nullptr;
			if ( is_singleton )
				{
				kp1 = kp0;
				int divider = strlen(kp0) + 1;
				re = new RE_Matcher(kp1, kp1 + divider);
				kp1 += k.Size();
				}
			else
				{
				const uint64_t* const len = AlignType<uint64_t>(kp0);

				kp1 = reinterpret_cast<const char*>(len+2);
				re = new RE_Matcher(kp1, kp1 + len[0]);
				kp1 += len[0] + len[1];
				}

			if ( ! re->Compile() )
				reporter->InternalError("failed compiling table/set key pattern: %s",
				                        re->PatternText());

			*pval = make_intrusive<PatternVal>(re);
			}
			break;

		case zeek::TYPE_RECORD:
			{
			const char* kp = kp0;
			zeek::RecordType* rt = t->AsRecordType();
			int num_fields = rt->NumFields();

			std::vector<IntrusivePtr<Val>> values;
			int i;
			for ( i = 0; i < num_fields; ++i )
				{
				IntrusivePtr<Val> v;

				zeek::detail::Attributes* a = rt->FieldDecl(i)->attrs.get();
				bool optional = (a && a->Find(zeek::detail::ATTR_OPTIONAL));

				kp = RecoverOneVal(k, kp, k_end,
				                   rt->GetFieldType(i).get(), &v, optional);

				// An earlier call to reporter->InternalError would have called abort() and broken the
				// call tree that clang-tidy is relying on to get the error described.
				// NOLINTNEXTLINE(clang-analyzer-core.uninitialized.Branch)
				if ( ! (v || optional) )
					{
					reporter->InternalError("didn't recover expected number of fields from HashKey");
					pval = nullptr;
					break;
					}

				values.emplace_back(std::move(v));
				}

			ASSERT(int(values.size()) == num_fields);

			auto rv = make_intrusive<RecordVal>(IntrusivePtr{NewRef{}, rt});

			for ( int i = 0; i < num_fields; ++i )
				rv->Assign(i, std::move(values[i]));

			*pval = std::move(rv);
			kp1 = kp;
			}
			break;

		case zeek::TYPE_TABLE:
			{
			int n;
			const int* const kp = AlignType<int>(kp0);
			n = *kp;
			kp1 = reinterpret_cast<const char*>(kp+1);
			zeek::TableType* tt = t->AsTableType();
			auto tv = make_intrusive<TableVal>(IntrusivePtr{NewRef{}, tt});

			for ( int i = 0; i < n; ++i )
				{
				IntrusivePtr<Val> key;
				kp1 = RecoverOneVal(k, kp1, k_end, tt->GetIndices().get(), &key, false);

				if ( t->IsSet() )
					tv->Assign(std::move(key), nullptr);
				else
					{
					IntrusivePtr<Val> value;
					kp1 = RecoverOneVal(k, kp1, k_end, tt->Yield().get(), &value,
					                    false);
					tv->Assign(std::move(key), std::move(value));
					}
				}

			*pval = std::move(tv);
			}
			break;

		case zeek::TYPE_VECTOR:
			{
			unsigned int n;
			const unsigned int* kp = AlignType<unsigned int>(kp0);
			n = *kp;
			kp1 = reinterpret_cast<const char*>(kp+1);
			zeek::VectorType* vt = t->AsVectorType();
			auto vv = make_intrusive<VectorVal>(IntrusivePtr{NewRef{}, vt});

			for ( unsigned int i = 0; i < n; ++i )
				{
				kp = AlignType<unsigned int>(kp1);
				unsigned int index = *kp;
				kp1 = reinterpret_cast<const char*>(kp+1);
				kp = AlignType<unsigned int>(kp1);
				unsigned int have_val = *kp;
				kp1 = reinterpret_cast<const char*>(kp+1);
				IntrusivePtr<Val> value;

				if ( have_val )
					kp1 = RecoverOneVal(k, kp1, k_end, vt->Yield().get(), &value,
					                    false);

				vv->Assign(index, std::move(value));
				}

			*pval = std::move(vv);
			}
			break;

		case zeek::TYPE_LIST:
			{
			int n;
			const int* const kp = AlignType<int>(kp0);
			n = *kp;
			kp1 = reinterpret_cast<const char*>(kp+1);
			zeek::TypeList* tl = t->AsTypeList();
			auto lv = make_intrusive<ListVal>(zeek::TYPE_ANY);

			for ( int i = 0; i < n; ++i )
				{
				IntrusivePtr<Val> v;
				zeek::Type* it = tl->Types()[i].get();
				kp1 = RecoverOneVal(k, kp1, k_end, it, &v, false);
				lv->Append(std::move(v));
				}

			*pval = std::move(lv);
			}
			break;

		default:
			{
			reporter->InternalError("bad index type in CompositeHash::DescribeKey");
			}
		}
		}
		break;

	case zeek::TYPE_INTERNAL_STRING:
		{
		// There is a minor issue here -- the pointer does not have to
		// be aligned by int in the singleton case.

		int n;
		if ( is_singleton )
			{
			kp1 = kp0;
			n = k.Size();
			}
		else
			{
			const int* const kp = AlignType<int>(kp0);
			n = *kp;
			kp1 = reinterpret_cast<const char*>(kp+1);
			}

		*pval = make_intrusive<StringVal>(new BroString((const byte_vec) kp1, n, true));
		kp1 += n;
		}
		break;

	case zeek::TYPE_INTERNAL_ERROR:
		break;
	}

	return kp1;
	}
