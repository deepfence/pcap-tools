// See the file "COPYING" in the main distribution directory for copyright.

#pragma once

#include "IntrusivePtr.h"
#include "Obj.h"
#include "Attr.h"
#include "Notifier.h"
#include "TraverseTypes.h"

#include <map>
#include <string>
#include <string_view>
#include <vector>

class Val;
class Func;

namespace zeek { class Type; }
using BroType [[deprecated("Remove in v4.1. Use zeek::Type instead.")]] = zeek::Type;

ZEEK_FORWARD_DECLARE_NAMESPACED(RecordType, zeek);
ZEEK_FORWARD_DECLARE_NAMESPACED(TableType, zeek);
ZEEK_FORWARD_DECLARE_NAMESPACED(VectorType, zeek);
ZEEK_FORWARD_DECLARE_NAMESPACED(EnumType, zeek);

enum [[deprecated("Remove in v4.1. Use zeek::detail::init_class instead.")]] init_class { INIT_NONE, INIT_FULL, INIT_EXTRA, INIT_REMOVE, };
enum [[deprecated("Remove in v4.1. Use zeek::detail::IDScope instead.")]] IDScope { SCOPE_FUNCTION, SCOPE_MODULE, SCOPE_GLOBAL };

namespace zeek::detail {

class Attributes;
class Expr;

enum init_class { INIT_NONE, INIT_FULL, INIT_EXTRA, INIT_REMOVE, };
enum IDScope { SCOPE_FUNCTION, SCOPE_MODULE, SCOPE_GLOBAL };

class ID final : public BroObj, public notifier::Modifiable {
public:
	static inline const IntrusivePtr<ID> nil;

	ID(const char* name, IDScope arg_scope, bool arg_is_export);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	[[deprecated("Remove in v4.1. Use version that takes zeek::detail::IDScope")]]
	ID(const char* name, ::IDScope arg_scope, bool arg_is_export);
#pragma GCC diagnostic pop

	~ID() override;

	const char* Name() const	{ return name; }

	int Scope() const		{ return scope; }
	bool IsGlobal() const           { return scope != SCOPE_FUNCTION; }

	bool IsExport() const           { return is_export; }
	void SetExport()                { is_export = true; }

	std::string ModuleName() const;

	void SetType(IntrusivePtr<Type> t);

	[[deprecated("Remove in v4.1.  Use GetType().")]]
	zeek::Type* Type()			{ return type.get(); }
	[[deprecated("Remove in v4.1.  Use GetType().")]]
	const zeek::Type* Type() const	{ return type.get(); }

	const IntrusivePtr<zeek::Type>& GetType() const
		{ return type; }

	template <class T>
	IntrusivePtr<T> GetType() const
		{ return cast_intrusive<T>(type); }

	[[deprecated("Remove in v4.1.  Use IsType() and GetType().")]]
	zeek::Type* AsType()		{ return is_type ? GetType().get() : nullptr; }
	[[deprecated("Remove in v4.1.  Use IsType() and GetType().")]]
	const zeek::Type* AsType() const	{ return is_type ? GetType().get() : nullptr; }

	bool IsType() const
		{ return is_type; }

	void MakeType()			{ is_type = true; }

	void SetVal(IntrusivePtr<Val> v);

	void SetVal(IntrusivePtr<Val> v, init_class c);
	void SetVal(IntrusivePtr<Expr> ev, init_class c);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	[[deprecated("Remove in v4.1. Use version that takes zeek::detail::init_class")]]
	void SetVal(IntrusivePtr<Val> v, ::init_class c);
	[[deprecated("Remove in v4.1. Use version that takes zeek::detail::init_class")]]
	void SetVal(IntrusivePtr<Expr> ev, ::init_class c);
#pragma GCC diagnostic pop

	bool HasVal() const		{ return val != nullptr; }

	[[deprecated("Remove in v4.1.  Use GetVal().")]]
	Val* ID_Val()			{ return val.get(); }
	[[deprecated("Remove in v4.1.  Use GetVal().")]]
	const Val* ID_Val() const	{ return val.get(); }

	const IntrusivePtr<Val>& GetVal() const
		{ return val; }

	void ClearVal();

	void SetConst()			{ is_const = true; }
	bool IsConst() const		{ return is_const; }

	void SetOption();
	bool IsOption() const		{ return is_option; }

	void SetEnumConst()		{ is_enum_const = true; }
	bool IsEnumConst() const		{ return is_enum_const; }

	void SetOffset(int arg_offset)	{ offset = arg_offset; }
	int Offset() const		{ return offset; }

	bool IsRedefinable() const;

	void SetAttrs(IntrusivePtr<Attributes> attr);
	void AddAttrs(IntrusivePtr<Attributes> attr);
	void RemoveAttr(attr_tag a);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	[[deprecated("Remove in v4.1. Use version that takes zeek::detail::attr_tag")]]
	void RemoveAttr(::attr_tag a);
#pragma GCC diagnostic pop
	void UpdateValAttrs();

	const IntrusivePtr<Attributes>& GetAttrs() const
		{ return attrs; }

	[[deprecated("Remove in 4.1.  Use GetAttrs().")]]
	Attributes* Attrs() const	{ return attrs.get(); }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	[[deprecated("Remove in 4.1.  Use GetAttr().")]]
	Attr* FindAttr(::attr_tag t) const
		{ return GetAttr(static_cast<zeek::detail::attr_tag>(t)).get(); }
#pragma GCC diagnostic pop

	const IntrusivePtr<zeek::detail::Attr>& GetAttr(zeek::detail::attr_tag t) const;

	bool IsDeprecated() const;

	void MakeDeprecated(IntrusivePtr<Expr> deprecation);

	std::string GetDeprecationWarning() const;

	void Error(const char* msg, const BroObj* o2 = nullptr);

	void Describe(ODesc* d) const override;
	// Adds type and value to description.
	void DescribeExtended(ODesc* d) const;
	// Produces a description that's reST-ready.
	void DescribeReST(ODesc* d, bool roles_only = false) const;
	void DescribeReSTShort(ODesc* d) const;

	bool DoInferReturnType() const
		{ return infer_return_type; }
	void SetInferReturnType(bool infer)
		{ infer_return_type = infer; }

	virtual TraversalCode Traverse(TraversalCallback* cb) const;

	bool HasOptionHandlers() const
		{ return !option_handlers.empty(); }

	void AddOptionHandler(IntrusivePtr<Func> callback, int priority);
	std::vector<Func*> GetOptionHandlers() const;

protected:
	void EvalFunc(IntrusivePtr<Expr> ef, IntrusivePtr<Expr> ev);

#ifdef DEBUG
	void UpdateValID();
#endif

	const char* name;
	IDScope scope;
	bool is_export;
	bool infer_return_type;
	IntrusivePtr<zeek::Type> type;
	bool is_const, is_enum_const, is_type, is_option;
	int offset;
	IntrusivePtr<Val> val;
	IntrusivePtr<Attributes> attrs;
	// contains list of functions that are called when an option changes
	std::multimap<int, IntrusivePtr<Func>> option_handlers;

};

}

using ID [[deprecated("Remove in v4.1. Use zeek::detail::ID instead.")]] = zeek::detail::ID;

namespace zeek::id {

/**
 * Lookup an ID in the global module and return it, if one exists;
 * @param name  The identifier name to lookup.
 * @return  The identifier, which may reference a nil object if no such
 * name exists.
 */
const IntrusivePtr<zeek::detail::ID>& find(std::string_view name);

/**
 * Lookup an ID by its name and return its type.  A fatal occurs if the ID
 * does not exist.
 * @param name  The identifier name to lookup
 * @return  The type of the identifier.
 */
const IntrusivePtr<zeek::Type>& find_type(std::string_view name);

/**
 * Lookup an ID by its name and return its type (as cast to @c T).
 * A fatal occurs if the ID does not exist.
 * @param name  The identifier name to lookup
 * @return  The type of the identifier.
 */
template<class T>
IntrusivePtr<T> find_type(std::string_view name)
	{ return cast_intrusive<T>(find_type(name)); }

/**
 * Lookup an ID by its name and return its value.  A fatal occurs if the ID
 * does not exist.
 * @param name  The identifier name to lookup
 * @return  The current value of the identifier
 */
const IntrusivePtr<Val>& find_val(std::string_view name);

/**
 * Lookup an ID by its name and return its value (as cast to @c T).
 * A fatal occurs if the ID does not exist.
 * @param name  The identifier name to lookup
 * @return  The current value of the identifier.
 */
template<class T>
IntrusivePtr<T> find_val(std::string_view name)
	{ return cast_intrusive<T>(find_val(name)); }

/**
 * Lookup an ID by its name and return its value.  A fatal occurs if the ID
 * does not exist or if it is not "const".
 * @param name  The identifier name to lookup
 * @return  The current value of the identifier
 */
const IntrusivePtr<Val>& find_const(std::string_view name);

/**
 * Lookup an ID by its name and return its value (as cast to @c T).
 * A fatal occurs if the ID does not exist.
 * @param name  The identifier name to lookup
 * @return  The current value of the identifier.
 */
template<class T>
IntrusivePtr<T> find_const(std::string_view name)
	{ return cast_intrusive<T>(find_const(name)); }

/**
 * Lookup an ID by its name and return the function it references.
 * A fatal occurs if the ID does not exist or if it is not a function.
 * @param name  The identifier name to lookup
 * @return  The current function value the identifier references.
 */
IntrusivePtr<Func> find_func(std::string_view name);

extern IntrusivePtr<zeek::RecordType> conn_id;
extern IntrusivePtr<zeek::RecordType> endpoint;
extern IntrusivePtr<zeek::RecordType> connection;
extern IntrusivePtr<zeek::RecordType> fa_file;
extern IntrusivePtr<zeek::RecordType> fa_metadata;
extern IntrusivePtr<zeek::EnumType> transport_proto;
extern IntrusivePtr<zeek::TableType> string_set;
extern IntrusivePtr<zeek::TableType> string_array;
extern IntrusivePtr<zeek::TableType> count_set;
extern IntrusivePtr<zeek::VectorType> string_vec;
extern IntrusivePtr<zeek::VectorType> index_vec;

namespace detail {

void init();

} // namespace zeek::id::detail

} // namespace zeek::id
