// See the file "COPYING" in the main distribution directory for copyright.

#pragma once

#include "Obj.h"
#include "Attr.h"
#include "BroList.h"
#include "IntrusivePtr.h"

#include <string>
#include <set>
#include <unordered_map>
#include <map>
#include <list>
#include <optional>

class EnumVal;
class TableVal;

ZEEK_FORWARD_DECLARE_NAMESPACED(Expr, zeek::detail);
ZEEK_FORWARD_DECLARE_NAMESPACED(ListExpr, zeek::detail);
ZEEK_FORWARD_DECLARE_NAMESPACED(Attributes, zeek::detail);

enum [[deprecated("Remove in v4.1. Use zeek::TypeTag instead.")]] TypeTag {
	TYPE_VOID,      // 0
	TYPE_BOOL,      // 1
	TYPE_INT,       // 2
	TYPE_COUNT,     // 3
	TYPE_COUNTER,   // 4
	TYPE_DOUBLE,    // 5
	TYPE_TIME,      // 6
	TYPE_INTERVAL,  // 7
	TYPE_STRING,    // 8
	TYPE_PATTERN,   // 9
	TYPE_ENUM,      // 10
	TYPE_TIMER,     // 11
	TYPE_PORT,      // 12
	TYPE_ADDR,      // 13
	TYPE_SUBNET,    // 14
	TYPE_ANY,       // 15
	TYPE_TABLE,     // 16
	TYPE_UNION,     // 17
	TYPE_RECORD,    // 18
	TYPE_LIST,      // 19
	TYPE_FUNC,      // 20
	TYPE_FILE,      // 21
	TYPE_VECTOR,    // 22
	TYPE_OPAQUE,    // 23
	TYPE_TYPE,      // 24
	TYPE_ERROR      // 25
#define NUM_TYPES (int(TYPE_ERROR) + 1)
};

enum [[deprecated("Remove in v4.1. Use zeek::FunctionFlavor instead.")]] function_flavor {
	FUNC_FLAVOR_FUNCTION,
	FUNC_FLAVOR_EVENT,
	FUNC_FLAVOR_HOOK
};

enum [[deprecated("Remove in v4.1. Use zeek::InternalTypeTag instead.")]] InternalTypeTag : uint16_t {
	TYPE_INTERNAL_VOID,
	TYPE_INTERNAL_INT, TYPE_INTERNAL_UNSIGNED, TYPE_INTERNAL_DOUBLE,
	TYPE_INTERNAL_STRING, TYPE_INTERNAL_ADDR, TYPE_INTERNAL_SUBNET,
	TYPE_INTERNAL_OTHER, TYPE_INTERNAL_ERROR
};

namespace zeek {

// BRO types.
enum TypeTag {
	TYPE_VOID,      // 0
	TYPE_BOOL,      // 1
	TYPE_INT,       // 2
	TYPE_COUNT,     // 3
	TYPE_COUNTER,   // 4
	TYPE_DOUBLE,    // 5
	TYPE_TIME,      // 6
	TYPE_INTERVAL,  // 7
	TYPE_STRING,    // 8
	TYPE_PATTERN,   // 9
	TYPE_ENUM,      // 10
	TYPE_TIMER,     // 11
	TYPE_PORT,      // 12
	TYPE_ADDR,      // 13
	TYPE_SUBNET,    // 14
	TYPE_ANY,       // 15
	TYPE_TABLE,     // 16
	TYPE_UNION,     // 17
	TYPE_RECORD,    // 18
	TYPE_LIST,      // 19
	TYPE_FUNC,      // 20
	TYPE_FILE,      // 21
	TYPE_VECTOR,    // 22
	TYPE_OPAQUE,    // 23
	TYPE_TYPE,      // 24
	TYPE_ERROR      // 25
#define NUM_TYPES (int(TYPE_ERROR) + 1)
};

// Returns the name of the type.
extern const char* type_name(TypeTag t);

constexpr bool is_network_order(TypeTag tag) noexcept
	{
	return tag == TYPE_PORT;
	}

enum FunctionFlavor {
	FUNC_FLAVOR_FUNCTION,
	FUNC_FLAVOR_EVENT,
	FUNC_FLAVOR_HOOK
};

enum InternalTypeTag : uint16_t {
	TYPE_INTERNAL_VOID,
	TYPE_INTERNAL_INT, TYPE_INTERNAL_UNSIGNED, TYPE_INTERNAL_DOUBLE,
	TYPE_INTERNAL_STRING, TYPE_INTERNAL_ADDR, TYPE_INTERNAL_SUBNET,
	TYPE_INTERNAL_OTHER, TYPE_INTERNAL_ERROR
};

constexpr InternalTypeTag to_internal_type_tag(TypeTag tag) noexcept
	{
	switch ( tag ) {
	case TYPE_VOID:
		return TYPE_INTERNAL_VOID;

	case TYPE_BOOL:
	case TYPE_INT:
	case TYPE_ENUM:
		return TYPE_INTERNAL_INT;

	case TYPE_COUNT:
	case TYPE_COUNTER:
	case TYPE_PORT:
		return TYPE_INTERNAL_UNSIGNED;

	case TYPE_DOUBLE:
	case TYPE_TIME:
	case TYPE_INTERVAL:
		return TYPE_INTERNAL_DOUBLE;

	case TYPE_STRING:
		return TYPE_INTERNAL_STRING;

	case TYPE_ADDR:
		return TYPE_INTERNAL_ADDR;

	case TYPE_SUBNET:
		return TYPE_INTERNAL_SUBNET;

	case TYPE_PATTERN:
	case TYPE_TIMER:
	case TYPE_ANY:
	case TYPE_TABLE:
	case TYPE_UNION:
	case TYPE_RECORD:
	case TYPE_LIST:
	case TYPE_FUNC:
	case TYPE_FILE:
	case TYPE_OPAQUE:
	case TYPE_VECTOR:
	case TYPE_TYPE:
		return TYPE_INTERNAL_OTHER;

	case TYPE_ERROR:
		return TYPE_INTERNAL_ERROR;
	}

	/* this should be unreachable */
	return TYPE_INTERNAL_VOID;
	}

class TypeList;
class TableType;
class SetType;
class RecordType;
class SubNetType;
class FuncType;
class EnumType;
class VectorType;
class TypeType;
class OpaqueType;

constexpr int DOES_NOT_MATCH_INDEX = 0;
constexpr int MATCHES_INDEX_SCALAR = 1;
constexpr int MATCHES_INDEX_VECTOR = 2;

class Type : public BroObj {
public:
	static inline const IntrusivePtr<Type> nil;

	explicit Type(zeek::TypeTag tag, bool base_type = false);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	[[deprecated("Remove in v4.1. Use the version that takes zeek::TypeTag instead.")]]
	explicit Type(::TypeTag tag, bool base_type = false)
		: Type(static_cast<zeek::TypeTag>(tag), base_type)
		{}
#pragma GCC diagnostic pop

	// Performs a shallow clone operation of the Bro type.
	// This especially means that especially for tables the types
	// are not recursively cloned; altering one type will in this case
	// alter one of them.
	// The main use for this is alias tracking.
	// Clone operations will mostly be implemented in the derived classes;
	// in addition cloning will be limited to classes that can be reached by
	// the script-level.
	virtual IntrusivePtr<Type> ShallowClone();

	TypeTag Tag() const		{ return tag; }
	InternalTypeTag InternalType() const	{ return internal_tag; }

	// Whether it's stored in network order.
	bool IsNetworkOrder() const	{ return is_network_order; }

	// Type-checks the given expression list, returning
	// MATCHES_INDEX_SCALAR = 1 if it matches this type's index
	// and produces a scalar result (and promoting its
	// subexpressions as necessary); MATCHES_INDEX_VECTOR = 2
	// if it matches and produces a vector result; and
	// DOES_NOT_MATCH_INDEX = 0 if it can't match (or the type
	// is not an indexable type).
	virtual int MatchesIndex(zeek::detail::ListExpr* index) const;

	// Returns the type yielded by this type.  For example, if
	// this type is a table[string] of port, then returns the "port"
	// type.  Returns nil if this is not an index type.
	virtual const IntrusivePtr<Type>& Yield() const;

	[[deprecated("Remove in v4.1.  Use Yield() instead.")]]
	virtual Type* YieldType()
		{ return Yield().get(); }
	[[deprecated("Remove in v4.1.  Use Yield() instead.")]]
	virtual const Type* YieldType() const
		{ return Yield().get(); }

	// Returns true if this type is a record and contains the
	// given field, false otherwise.
	[[deprecated("Remove in v4.1.  Use RecordType::HasField() directly.")]]
	virtual bool HasField(const char* field) const;

	// Returns the type of the given field, or nil if no such field.
	[[deprecated("Remove in v4.1.  Use RecordType::GetFieldType() directly.")]]
	virtual Type* FieldType(const char* field) const;

#define CHECK_TYPE_TAG(tag_type, func_name) \
	CHECK_TAG(tag, tag_type, func_name, type_name)

	const TypeList* AsTypeList() const
		{
		CHECK_TYPE_TAG(TYPE_LIST, "Type::AsTypeList");
		return (const TypeList*) this;
		}
	TypeList* AsTypeList()
		{
		CHECK_TYPE_TAG(TYPE_LIST, "Type::AsTypeList");
		return (TypeList*) this;
		}

	const TableType* AsTableType() const
		{
		CHECK_TYPE_TAG(TYPE_TABLE, "Type::AsTableType");
		return (const TableType*) this;
		}
	TableType* AsTableType()
		{
		CHECK_TYPE_TAG(TYPE_TABLE, "Type::AsTableType");
		return (TableType*) this;
		}

	SetType* AsSetType()
		{
		if ( ! IsSet() )
			BadTag("Type::AsSetType", type_name(tag));
		return (SetType*) this;
		}
	const SetType* AsSetType() const
		{
		if ( ! IsSet() )
			BadTag("Type::AsSetType", type_name(tag));
		return (const SetType*) this;
		}

	const RecordType* AsRecordType() const
		{
		CHECK_TYPE_TAG(TYPE_RECORD, "Type::AsRecordType");
		return (const RecordType*) this;
		}
	RecordType* AsRecordType()
		{
		CHECK_TYPE_TAG(TYPE_RECORD, "Type::AsRecordType");
		return (RecordType*) this;
		}

	const SubNetType* AsSubNetType() const
		{
		CHECK_TYPE_TAG(TYPE_SUBNET, "Type::AsSubNetType");
		return (const SubNetType*) this;
		}

	SubNetType* AsSubNetType()
		{
		CHECK_TYPE_TAG(TYPE_SUBNET, "Type::AsSubNetType");
		return (SubNetType*) this;
		}

	const FuncType* AsFuncType() const
		{
		CHECK_TYPE_TAG(TYPE_FUNC, "Type::AsFuncType");
		return (const FuncType*) this;
		}

	FuncType* AsFuncType()
		{
		CHECK_TYPE_TAG(TYPE_FUNC, "Type::AsFuncType");
		return (FuncType*) this;
		}

	const EnumType* AsEnumType() const
		{
		CHECK_TYPE_TAG(TYPE_ENUM, "Type::AsEnumType");
		return (EnumType*) this;
		}

	EnumType* AsEnumType()
		{
		CHECK_TYPE_TAG(TYPE_ENUM, "Type::AsEnumType");
		return (EnumType*) this;
		}

	const VectorType* AsVectorType() const
		{
		CHECK_TYPE_TAG(TYPE_VECTOR, "Type::AsVectorType");
		return (VectorType*) this;
		}

	OpaqueType* AsOpaqueType()
		{
		CHECK_TYPE_TAG(TYPE_OPAQUE, "Type::AsOpaqueType");
		return (OpaqueType*) this;
		}

	const OpaqueType* AsOpaqueType() const
		{
		CHECK_TYPE_TAG(TYPE_OPAQUE, "Type::AsOpaqueType");
		return (OpaqueType*) this;
		}

	VectorType* AsVectorType()
		{
		CHECK_TYPE_TAG(TYPE_VECTOR, "Type::AsVectorType");
		return (VectorType*) this;
		}

	const TypeType* AsTypeType() const
		{
		CHECK_TYPE_TAG(TYPE_TYPE, "Type::AsTypeType");
		return (TypeType*) this;
		}

	TypeType* AsTypeType()
		{
		CHECK_TYPE_TAG(TYPE_TYPE, "Type::AsTypeType");
		return (TypeType*) this;
		}

	bool IsSet() const
		{
		return tag == TYPE_TABLE && ! Yield();
		}

	bool IsTable() const
		{
		return tag == TYPE_TABLE && Yield();
		}

	Type* Ref()		{ ::Ref(this); return this; }

	void Describe(ODesc* d) const override;
	virtual void DescribeReST(ODesc* d, bool roles_only = false) const;

	virtual unsigned MemoryAllocation() const;

	void SetName(const std::string& arg_name) { name = arg_name; }
	const std::string& GetName() const { return name; }

	typedef std::map<std::string, std::set<Type*> > TypeAliasMap;

	static std::set<Type*> GetAliases(const std::string& type_name)
		{ return Type::type_aliases[type_name]; }

	static void AddAlias(const std::string &type_name, Type* type)
		{ Type::type_aliases[type_name].insert(type); }

protected:
	Type() = default;

	void SetError();

private:
	TypeTag tag;
	InternalTypeTag internal_tag;
	bool is_network_order;
	bool base_type;
	std::string name;

	static TypeAliasMap type_aliases;
};

class TypeList final : public Type {
public:
	explicit TypeList(IntrusivePtr<Type> arg_pure_type = nullptr)
		: Type(TYPE_LIST), pure_type(std::move(arg_pure_type))
		{
		}

	const std::vector<IntrusivePtr<Type>>& Types() const
		{ return types; }

	bool IsPure() const		{ return pure_type != nullptr; }

	// Returns the underlying pure type, or nil if the list
	// is not pure or is empty.
	const IntrusivePtr<Type>& GetPureType() const
		{ return pure_type; }

	[[deprecated("Remove in v4.1.  Use GetPureType() instead.")]]
	Type* PureType()		{ return pure_type.get(); }
	[[deprecated("Remove in v4.1.  Use GetPureType() instead.")]]
	const Type* PureType() const	{ return pure_type.get(); }

	// True if all of the types match t, false otherwise.  If
	// is_init is true, then the matching is done in the context
	// of an initialization.
	bool AllMatch(const Type* t, bool is_init) const;
	bool AllMatch(const IntrusivePtr<Type>& t, bool is_init) const
		{ return AllMatch(t.get(), is_init); }

	void Append(IntrusivePtr<Type> t);
	void AppendEvenIfNotPure(IntrusivePtr<Type> t);

	void Describe(ODesc* d) const override;

	unsigned int MemoryAllocation() const override;

protected:
	IntrusivePtr<Type> pure_type;
	std::vector<IntrusivePtr<Type>> types;
};

class IndexType : public Type {
public:
	int MatchesIndex(zeek::detail::ListExpr* index) const override;

	const IntrusivePtr<TypeList>& GetIndices() const
		{ return indices; }

	[[deprecated("Remove in v4.1.  Use GetIndices().")]]
	TypeList* Indices() const		{ return indices.get(); }

	const std::vector<IntrusivePtr<Type>>& IndexTypes() const
		{ return indices->Types(); }

	const IntrusivePtr<Type>& Yield() const override
		{ return yield_type; }

	void Describe(ODesc* d) const override;
	void DescribeReST(ODesc* d, bool roles_only = false) const override;

	// Returns true if this table is solely indexed by subnet.
	bool IsSubNetIndex() const;

protected:
	IndexType(TypeTag t, IntrusivePtr<TypeList> arg_indices,
	          IntrusivePtr<Type> arg_yield_type)
		: Type(t), indices(std::move(arg_indices)),
		  yield_type(std::move(arg_yield_type))
		{
		}

	~IndexType() override;

	IntrusivePtr<TypeList> indices;
	IntrusivePtr<Type> yield_type;
};

class TableType : public IndexType {
public:
	TableType(IntrusivePtr<TypeList> ind, IntrusivePtr<Type> yield);

	IntrusivePtr<Type> ShallowClone() override;

	// Returns true if this table type is "unspecified", which is
	// what one gets using an empty "set()" or "table()" constructor.
	bool IsUnspecifiedTable() const;
};

class SetType final : public TableType {
public:
	SetType(IntrusivePtr<TypeList> ind, IntrusivePtr<zeek::detail::ListExpr> arg_elements);
	~SetType() override;

	IntrusivePtr<Type> ShallowClone() override;

	[[deprecated("Remove in v4.1.  Use Elements() isntead.")]]
	zeek::detail::ListExpr* SetElements() const	{ return elements.get(); }

	const IntrusivePtr<zeek::detail::ListExpr>& Elements() const
		{ return elements; }

protected:
	IntrusivePtr<zeek::detail::ListExpr> elements;
};

class FuncType final : public Type {
public:
	static inline const IntrusivePtr<FuncType> nil;

	/**
	 * Prototype is only currently used for events and hooks which declare
	 * multiple signature prototypes that allow users to have handlers
	 * with various argument permutations.
	 */
	struct Prototype {
		bool deprecated;
		IntrusivePtr<RecordType> args;
		std::map<int, int> offsets;
	};

	FuncType(IntrusivePtr<RecordType> args, IntrusivePtr<Type> yield,
	         FunctionFlavor f);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	[[deprecated("Remove in v4.1. Use the version that takes zeek::FunctionFlavor instead.")]]
	FuncType(IntrusivePtr<RecordType> args, IntrusivePtr<Type> yield, ::function_flavor f)
		: FuncType(args, yield, static_cast<FunctionFlavor>(f))
		{}
#pragma GCC diagnostic pop

	IntrusivePtr<Type> ShallowClone() override;

	~FuncType() override;

	[[deprecated("Remove in v4.1.  Use Params().")]]
	RecordType* Args() const	{ return args.get(); }

	const IntrusivePtr<RecordType>& Params() const
		{ return args; }

	const IntrusivePtr<Type>& Yield() const override
		{ return yield; }

	void SetYieldType(IntrusivePtr<Type> arg_yield)	{ yield = std::move(arg_yield); }
	FunctionFlavor Flavor() const { return flavor; }
	std::string FlavorString() const;

	// Used to convert a function type to an event or hook type.
	void ClearYieldType(FunctionFlavor arg_flav)
		{ yield = nullptr; flavor = arg_flav; }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	[[deprecated("Remove in v4.1. Use the version that takes zeek::FunctionFlavor instead.")]]
	void ClearYieldType(::function_flavor arg_flav)
		{ yield = nullptr; flavor = static_cast<FunctionFlavor>(arg_flav); }
#pragma GCC diagnostic pop

	int MatchesIndex(zeek::detail::ListExpr* index) const override;
	bool CheckArgs(const type_list* args, bool is_init = false) const;
	bool CheckArgs(const std::vector<IntrusivePtr<Type>>& args,
	               bool is_init = false) const;

	[[deprecated("Remove in v4.1.  Use ParamList().")]]
	TypeList* ArgTypes() const	{ return arg_types.get(); }

	const IntrusivePtr<TypeList>& ParamList() const
		{ return arg_types; }

	void Describe(ODesc* d) const override;
	void DescribeReST(ODesc* d, bool roles_only = false) const override;

	/**
	 * Adds a new event/hook signature allowed for use in handlers.
	 */
	void AddPrototype(Prototype s);

	/**
	 * Returns a prototype signature that matches the desired argument types.
	 */
	std::optional<Prototype> FindPrototype(const RecordType& args) const;

	/**
	 * Returns all allowed function prototypes.
	 */
	const std::vector<Prototype>& Prototypes() const
		{ return prototypes; }

protected:
	friend IntrusivePtr<FuncType> make_intrusive<FuncType>();

	FuncType() : Type(TYPE_FUNC) { flavor = FUNC_FLAVOR_FUNCTION; }
	IntrusivePtr<RecordType> args;
	IntrusivePtr<TypeList> arg_types;
	IntrusivePtr<Type> yield;
	FunctionFlavor flavor;
	std::vector<Prototype> prototypes;
};

class TypeType final : public Type {
public:
	explicit TypeType(IntrusivePtr<Type> t) : zeek::Type(TYPE_TYPE), type(std::move(t)) {}
	IntrusivePtr<Type> ShallowClone() override { return make_intrusive<TypeType>(type); }

	const IntrusivePtr<Type>& GetType() const
		{ return type; }

	template <class T>
	IntrusivePtr<T> GetType() const
		{ return cast_intrusive<T>(type); }

	[[deprecated("Remove in v4.1.  Use GetType().")]]
	zeek::Type* Type()			{ return type.get(); }
	[[deprecated("Remove in v4.1.  Use GetType().")]]
	const zeek::Type* Type() const	{ return type.get(); }

protected:
	IntrusivePtr<zeek::Type> type;
};

class TypeDecl final {
public:
	TypeDecl() = default;
	TypeDecl(const char* i, IntrusivePtr<Type> t,
	         IntrusivePtr<zeek::detail::Attributes> attrs = nullptr);
	TypeDecl(const TypeDecl& other);
	~TypeDecl();

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	[[deprecated("Remove in v4.1.  Use GetAttr().")]]
	const zeek::detail::Attr* FindAttr(::attr_tag a) const
		{ return attrs ? attrs->Find(static_cast<zeek::detail::attr_tag>(a)).get() : nullptr; }
#pragma GCC diagnostic pop

	const IntrusivePtr<zeek::detail::Attr>& GetAttr(zeek::detail::attr_tag a) const
		{ return attrs ? attrs->Find(a) : zeek::detail::Attr::nil; }

	void DescribeReST(ODesc* d, bool roles_only = false) const;

	IntrusivePtr<Type> type;
	IntrusivePtr<zeek::detail::Attributes> attrs;
	const char* id = nullptr;
};

using type_decl_list = PList<TypeDecl>;

class RecordType final : public Type {
public:
	explicit RecordType(type_decl_list* types);
	IntrusivePtr<Type> ShallowClone() override;

	~RecordType() override;

	bool HasField(const char* field) const override;

	[[deprecated("Remove in v4.1.  Use GetFieldType() instead (note it doesn't check for invalid names).")]]
	Type* FieldType(const char* field) const override
		{
		auto offset = FieldOffset(field);
		return offset >= 0 ? GetFieldType(offset).get() : nullptr;
		}

	[[deprecated("Remove in v4.1.  Use GetFieldType() instead.")]]
	Type* FieldType(int field) const
		{ return GetFieldType(field).get(); }

	/**
	 * Looks up a field by name and returns its type.  No check for invalid
	 * field name is performed.
	 */
	const IntrusivePtr<Type>& GetFieldType(const char* field_name) const
		{ return GetFieldType(FieldOffset(field_name)); }

	/**
	 * Looks up a field by name and returns its type as cast to @c T.
	 * No check for invalid field name is performed.
	 */
	template <class T>
	IntrusivePtr<T> GetFieldType(const char* field_name) const
		{ return cast_intrusive<T>(GetFieldType(field_name)); }

	/**
	 * Looks up a field by its index and returns its type.  No check for
	 * invalid field offset is performed.
	 */
	const IntrusivePtr<Type>& GetFieldType(int field_index) const
		{ return (*types)[field_index]->type; }

	/**
	 * Looks up a field by its index and returns its type as cast to @c T.
	 * No check for invalid field offset is performed.
	 */
	template <class T>
	IntrusivePtr<T> GetFieldType(int field_index) const
		{ return cast_intrusive<T>((*types)[field_index]->type); }

	IntrusivePtr<Val> FieldDefault(int field) const;

	// A field's offset is its position in the type_decl_list,
	// starting at 0.  Returns negative if the field doesn't exist.
	int FieldOffset(const char* field) const;

	// Given an offset, returns the field's name.
	const char* FieldName(int field) const;

	type_decl_list* Types() { return types; }

	// Given an offset, returns the field's TypeDecl.
	const TypeDecl* FieldDecl(int field) const;
	TypeDecl* FieldDecl(int field);

	int NumFields() const			{ return num_fields; }

	/**
	 * Returns a "record_field_table" value for introspection purposes.
	 * @param rv  an optional record value, if given the values of
	 * all fields will be provided in the returned table.
	 */
	IntrusivePtr<TableVal> GetRecordFieldsVal(const RecordVal* rv = nullptr) const;

	// Returns null if all is ok, otherwise a pointer to an error message.
	const char* AddFields(const type_decl_list& types,
	                      bool add_log_attr = false);

	void Describe(ODesc* d) const override;
	void DescribeReST(ODesc* d, bool roles_only = false) const override;
	void DescribeFields(ODesc* d) const;
	void DescribeFieldsReST(ODesc* d, bool func_args) const;

	bool IsFieldDeprecated(int field) const
		{
		const TypeDecl* decl = FieldDecl(field);
		return decl && decl->GetAttr(zeek::detail::ATTR_DEPRECATED) != nullptr;
		}

	bool FieldHasAttr(int field, zeek::detail::attr_tag at) const
		{
		const TypeDecl* decl = FieldDecl(field);
		return decl && decl->GetAttr(at) != nullptr;
		}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	[[deprecated("Remove in v4.1. Use version that takes zeek::detail::attr_tag.")]]
	bool FieldHasAttr(int field, ::attr_tag at) const
		{
		return FieldHasAttr(field, static_cast<zeek::detail::attr_tag>(at));
		}
#pragma GCC diagnostic pop

	std::string GetFieldDeprecationWarning(int field, bool has_check) const;

protected:
	RecordType() { types = nullptr; }

	int num_fields;
	type_decl_list* types;
};

class SubNetType final : public Type {
public:
	SubNetType();
	void Describe(ODesc* d) const override;
};

class FileType final : public Type {
public:
	explicit FileType(IntrusivePtr<Type> yield_type);
	IntrusivePtr<Type> ShallowClone() override { return make_intrusive<FileType>(yield); }
	~FileType() override;

	const IntrusivePtr<Type>& Yield() const override
		{ return yield; }

	void Describe(ODesc* d) const override;

protected:
	IntrusivePtr<Type> yield;
};

class OpaqueType final : public Type {
public:
	explicit OpaqueType(const std::string& name);
	IntrusivePtr<Type> ShallowClone() override { return make_intrusive<OpaqueType>(name); }
	~OpaqueType() override { };

	const std::string& Name() const { return name; }

	void Describe(ODesc* d) const override;
	void DescribeReST(ODesc* d, bool roles_only = false) const override;

protected:
	OpaqueType() { }

	std::string name;
};

class EnumType final : public Type {
public:
	typedef std::list<std::pair<std::string, bro_int_t> > enum_name_list;

	explicit EnumType(const EnumType* e);
	explicit EnumType(const std::string& arg_name);
	IntrusivePtr<Type> ShallowClone() override;
	~EnumType() override;

	// The value of this name is next internal counter value, starting
	// with zero. The internal counter is incremented.
	void AddName(const std::string& module_name, const char* name, bool is_export, zeek::detail::Expr* deprecation = nullptr);

	// The value of this name is set to val. Once a value has been
	// explicitly assigned using this method, no further names can be
	// added that aren't likewise explicitly initalized.
	void AddName(const std::string& module_name, const char* name, bro_int_t val, bool is_export, zeek::detail::Expr* deprecation = nullptr);

	// -1 indicates not found.
	bro_int_t Lookup(const std::string& module_name, const char* name) const;
	const char* Lookup(bro_int_t value) const; // Returns 0 if not found

	// Returns the list of defined names with their values. The names
	// will be fully qualified with their module name.
	enum_name_list Names() const;

	void DescribeReST(ODesc* d, bool roles_only = false) const override;

	const IntrusivePtr<EnumVal>& GetVal(bro_int_t i);

protected:
	void AddNameInternal(const std::string& module_name,
			const char* name, bro_int_t val, bool is_export);

	void CheckAndAddName(const std::string& module_name,
	                     const char* name, bro_int_t val, bool is_export,
	                     zeek::detail::Expr* deprecation = nullptr);

	typedef std::map<std::string, bro_int_t> NameMap;
	NameMap names;

	using ValMap = std::unordered_map<bro_int_t, IntrusivePtr<EnumVal>>;
	ValMap vals;

	// The counter is initialized to 0 and incremented on every implicit
	// auto-increment name that gets added (thus its > 0 if
	// auto-increment is used).  Once an explicit value has been
	// specified, the counter is set to -1. This way counter can be used
	// as a flag to prevent mixing of auto-increment and explicit
	// enumerator specifications.
	bro_int_t counter;
};

class VectorType final : public Type {
public:
	explicit VectorType(IntrusivePtr<Type> t);
	IntrusivePtr<Type> ShallowClone() override;
	~VectorType() override;

	const IntrusivePtr<Type>& Yield() const override;

	int MatchesIndex(zeek::detail::ListExpr* index) const override;

	// Returns true if this table type is "unspecified", which is what one
	// gets using an empty "vector()" constructor.
	bool IsUnspecifiedVector() const;

	void Describe(ODesc* d) const override;
	void DescribeReST(ODesc* d, bool roles_only = false) const override;

protected:
	IntrusivePtr<Type> yield_type;
};

// True if the two types are equivalent.  If is_init is true then the test is
// done in the context of an initialization. If match_record_field_names is
// true then for record types the field names have to match, too.
extern bool same_type(const Type& t1, const Type& t2,
                      bool is_init=false, bool match_record_field_names=true);
inline bool same_type(const IntrusivePtr<Type>& t1, const IntrusivePtr<Type>& t2,
                      bool is_init=false, bool match_record_field_names=true)
    { return same_type(*t1, *t2, is_init, match_record_field_names); }
inline bool same_type(const Type* t1, const Type* t2,
                      bool is_init=false, bool match_record_field_names=true)
    { return same_type(*t1, *t2, is_init, match_record_field_names); }
inline bool same_type(const IntrusivePtr<Type>& t1, const Type* t2,
                      bool is_init=false, bool match_record_field_names=true)
    { return same_type(*t1, *t2, is_init, match_record_field_names); }
inline bool same_type(const Type* t1, const IntrusivePtr<Type>& t2,
                      bool is_init=false, bool match_record_field_names=true)
    { return same_type(*t1, *t2, is_init, match_record_field_names); }

// True if the two attribute lists are equivalent.
extern bool same_attrs(const zeek::detail::Attributes* a1, const zeek::detail::Attributes* a2);

// Returns true if the record sub_rec can be promoted to the record
// super_rec.
extern bool record_promotion_compatible(const RecordType* super_rec,
					const RecordType* sub_rec);

// If the given Type is a TypeList with just one element, returns
// that element, otherwise returns the type.
extern const Type* flatten_type(const Type* t);
extern Type* flatten_type(Type* t);

// Returns the "maximum" of two type tags, in a type-promotion sense.
extern TypeTag max_type(TypeTag t1, TypeTag t2);

// Given two types, returns the "merge", in which promotable types
// are promoted to the maximum of the two.  Returns nil (and generates
// an error message) if the types are incompatible.
IntrusivePtr<Type> merge_types(const IntrusivePtr<Type>& t1,
                                  const IntrusivePtr<Type>& t2);

// Given a list of expressions, returns a (ref'd) type reflecting
// a merged type consistent across all of them, or nil if this
// cannot be done.
IntrusivePtr<Type> merge_type_list(zeek::detail::ListExpr* elements);

// Given an expression, infer its type when used for an initialization.
IntrusivePtr<Type> init_type(zeek::detail::Expr* init);

// Returns true if argument is an atomic type.
bool is_atomic_type(const Type& t);
inline bool is_atomic_type(const Type* t)
	{ return is_atomic_type(*t); }
inline bool is_atomic_type(const IntrusivePtr<Type>& t)
	{ return is_atomic_type(*t); }

// True if the given type tag corresponds to type that can be assigned to.
extern bool is_assignable(TypeTag t);
inline bool is_assignable(Type* t)
	{ return zeek::is_assignable(t->Tag()); }

// True if the given type tag corresponds to an integral type.
inline bool IsIntegral(TypeTag t) { return (t == TYPE_INT || t == TYPE_COUNT || t == TYPE_COUNTER); }

// True if the given type tag corresponds to an arithmetic type.
inline bool IsArithmetic(TypeTag t)	{ return (IsIntegral(t) || t == TYPE_DOUBLE); }

// True if the given type tag corresponds to a boolean type.
inline bool IsBool(TypeTag t)	{ return (t == TYPE_BOOL); }

// True if the given type tag corresponds to an interval type.
inline bool IsInterval(TypeTag t)	{ return (t == TYPE_INTERVAL); }

// True if the given type tag corresponds to a record type.
inline bool IsRecord(TypeTag t)	{ return (t == TYPE_RECORD || t == TYPE_UNION); }

// True if the given type tag corresponds to a function type.
inline bool IsFunc(TypeTag t)	{ return (t == TYPE_FUNC); }

// True if the given type type is a vector.
inline bool IsVector(TypeTag t)	{ return (t == TYPE_VECTOR); }

// True if the given type type is a string.
inline bool IsString(TypeTag t)	{ return (t == TYPE_STRING); }

// True if the given type tag corresponds to the error type.
inline bool IsErrorType(TypeTag t)	{ return (t == TYPE_ERROR); }

// True if both tags are integral types.
inline bool BothIntegral(TypeTag t1, TypeTag t2) { return (IsIntegral(t1) && IsIntegral(t2)); }

// True if both tags are arithmetic types.
inline bool BothArithmetic(TypeTag t1, TypeTag t2) { return (IsArithmetic(t1) && IsArithmetic(t2)); }

// True if either tags is an arithmetic type.
inline bool EitherArithmetic(TypeTag t1, TypeTag t2) { return (IsArithmetic(t1) || IsArithmetic(t2)); }

// True if both tags are boolean types.
inline bool BothBool(TypeTag t1, TypeTag t2) { return (IsBool(t1) && IsBool(t2)); }

// True if both tags are interval types.
inline bool BothInterval(TypeTag t1, TypeTag t2) { return (IsInterval(t1) && IsInterval(t2)); }

// True if both tags are string types.
inline bool BothString(TypeTag t1, TypeTag t2) { return (IsString(t1) && IsString(t2)); }

// True if either tag is the error type.
inline bool EitherError(TypeTag t1, TypeTag t2) { return (IsErrorType(t1) || IsErrorType(t2)); }

// Returns the basic (non-parameterized) type with the given type.
const IntrusivePtr<zeek::Type>& base_type(zeek::TypeTag tag);

// Returns the basic error type.
inline const IntrusivePtr<zeek::Type>& error_type()       { return base_type(TYPE_ERROR); }

} // namespace zeek

// Returns the basic (non-parameterized) type with the given type.
// The reference count of the type is not increased.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
[[deprecated("Remove in v4.1.  Use zeek::base_type() instead")]]
inline zeek::Type* base_type_no_ref(::TypeTag tag)
	{ return zeek::base_type(static_cast<zeek::TypeTag>(tag)).get(); }
#pragma GCC diagnostic pop

extern IntrusivePtr<zeek::OpaqueType> md5_type;
extern IntrusivePtr<zeek::OpaqueType> sha1_type;
extern IntrusivePtr<zeek::OpaqueType> sha256_type;
extern IntrusivePtr<zeek::OpaqueType> entropy_type;
extern IntrusivePtr<zeek::OpaqueType> cardinality_type;
extern IntrusivePtr<zeek::OpaqueType> topk_type;
extern IntrusivePtr<zeek::OpaqueType> bloomfilter_type;
extern IntrusivePtr<zeek::OpaqueType> x509_opaque_type;
extern IntrusivePtr<zeek::OpaqueType> ocsp_resp_opaque_type;
extern IntrusivePtr<zeek::OpaqueType> paraglob_type;

using BroType [[deprecated("Remove in v4.1. Use zeek::Type instead.")]] = zeek::Type;
using TypeList [[deprecated("Remove in v4.1. Use zeek::TypeList instead.")]] = zeek::TypeList;
using IndexType [[deprecated("Remove in v4.1. Use zeek::IndexType instead.")]] = zeek::IndexType;
using TableType [[deprecated("Remove in v4.1. Use zeek::TableType instead.")]] = zeek::TableType;
using SetType [[deprecated("Remove in v4.1. Use zeek::SetType instead.")]] = zeek::SetType;
using FuncType [[deprecated("Remove in v4.1. Use zeek::FuncType instead.")]] = zeek::FuncType;
using TypeType [[deprecated("Remove in v4.1. Use zeek::TypeType instead.")]] = zeek::TypeType;
using TypeDecl [[deprecated("Remove in v4.1. Use zeek::TypeDecl instead.")]] = zeek::TypeDecl;
using RecordType [[deprecated("Remove in v4.1. Use zeek::RecordType instead.")]] = zeek::RecordType;
using SubNetType [[deprecated("Remove in v4.1. Use zeek::SubNetType instead.")]] = zeek::SubNetType;
using FileType [[deprecated("Remove in v4.1. Use zeek::FileType instead.")]] = zeek::FileType;
using OpaqueType [[deprecated("Remove in v4.1. Use zeek::OpaqueType instead.")]] = zeek::OpaqueType;
using EnumType [[deprecated("Remove in v4.1. Use zeek::EnumType instead.")]] = zeek::EnumType;
using VectorType [[deprecated("Remove in v4.1. Use zeek::VectorType instead.")]] = zeek::VectorType;
using type_decl_list [[deprecated("Remove in v4.1. Use zeek::type_decl_list instead.")]] = zeek::type_decl_list;

constexpr auto IsIntegral [[deprecated("Remove in v4.1. Use zeek::IsIntegral instead.")]] = zeek::IsIntegral;
constexpr auto IsArithmetic [[deprecated("Remove in v4.1. Use zeek::IsArithmetic instead.")]] = zeek::IsArithmetic;
constexpr auto IsBool [[deprecated("Remove in v4.1. Use zeek::IsBool instead.")]] = zeek::IsBool;
constexpr auto IsInterval [[deprecated("Remove in v4.1. Use zeek::IsInterval instead.")]] = zeek::IsInterval;
constexpr auto IsRecord [[deprecated("Remove in v4.1. Use zeek::IsRecord instead.")]] = zeek::IsRecord;
constexpr auto IsFunc [[deprecated("Remove in v4.1. Use zeek::IsFunc instead.")]] = zeek::IsFunc;
constexpr auto IsVector [[deprecated("Remove in v4.1. Use zeek::IsVector instead.")]] = zeek::IsVector;
constexpr auto IsString [[deprecated("Remove in v4.1. Use zeek::IsString instead.")]] = zeek::IsString;
constexpr auto IsErrorType [[deprecated("Remove in v4.1. Use zeek::IsErrorType instead.")]] = zeek::IsErrorType;
constexpr auto BothIntegral [[deprecated("Remove in v4.1. Use zeek::BothIntegral instead.")]] = zeek::BothIntegral;
constexpr auto BothArithmetic [[deprecated("Remove in v4.1. Use zeek::BothArithmetic instead.")]] = zeek::BothArithmetic;
constexpr auto EitherArithmetic [[deprecated("Remove in v4.1. Use zeek::EitherArithmetic instead.")]] = zeek::EitherArithmetic;
constexpr auto BothBool [[deprecated("Remove in v4.1. Use zeek::BothBool instead.")]] = zeek::BothBool;
constexpr auto BothInterval [[deprecated("Remove in v4.1. Use zeek::BothInterval instead.")]] = zeek::BothInterval;
constexpr auto BothString [[deprecated("Remove in v4.1. Use zeek::BothString instead.")]] = zeek::BothString;
constexpr auto EitherError [[deprecated("Remove in v4.1. Use zeek::EitherError instead.")]] = zeek::EitherError;
constexpr auto base_type [[deprecated("Remove in v4.1. Use zeek::base_type instead.")]] = zeek::base_type;
constexpr auto error_type [[deprecated("Remove in v4.1. Use zeek::error_type instead.")]] = zeek::error_type;
constexpr auto type_name [[deprecated("Remove in v4.1. Use zeek::type_name instead.")]] = zeek::type_name;
constexpr auto is_network_order [[deprecated("Remove in v4.1. Use zeek::is_network_order instead.")]] = zeek::is_network_order;
