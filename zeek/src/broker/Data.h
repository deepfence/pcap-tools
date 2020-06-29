#pragma once

#include "OpaqueVal.h"
#include "Reporter.h"
#include "Frame.h"
#include "Expr.h"

template <class T>
class IntrusivePtr;

class ODesc;

namespace threading {
struct Value;
struct Field;
}

namespace bro_broker {

extern IntrusivePtr<zeek::OpaqueType> opaque_of_data_type;
extern IntrusivePtr<zeek::OpaqueType> opaque_of_set_iterator;
extern IntrusivePtr<zeek::OpaqueType> opaque_of_table_iterator;
extern IntrusivePtr<zeek::OpaqueType> opaque_of_vector_iterator;
extern IntrusivePtr<zeek::OpaqueType> opaque_of_record_iterator;

/**
 * Convert a broker port protocol to a bro port protocol.
 */
TransportProto to_bro_port_proto(broker::port::protocol tp);

/**
 * Create a Broker::Data value from a Bro value.
 * @param v the Bro value to convert to a Broker data value.
 * @return a Broker::Data value, where the optional field is set if the conversion
 * was possible, else it is unset.
 */
IntrusivePtr<RecordVal> make_data_val(Val* v);

/**
 * Create a Broker::Data value from a Broker data value.
 * @param d the Broker value to wrap in an opaque type.
 * @return a Broker::Data value that wraps the Broker value.
 */
IntrusivePtr<RecordVal> make_data_val(broker::data d);

/**
 * Get the type of Broker data that Broker::Data wraps.
 * @param v a Broker::Data value.
 * @param frame used to get location info upon error.
 * @return a Broker::DataType value.
 */
IntrusivePtr<EnumVal> get_data_type(RecordVal* v, Frame* frame);

/**
 * Convert a Bro value to a Broker data value.
 * @param v a Bro value.
 * @return a Broker data value if the Bro value could be converted to one.
 */
broker::expected<broker::data> val_to_data(const Val* v);

/**
 * Convert a Broker data value to a Bro value.
 * @param d a Broker data value.
 * @param type the expected type of the value to return.
 * @return a pointer to a new Bro value or a nullptr if the conversion was not
 * possible.
 */
IntrusivePtr<Val> data_to_val(broker::data d, zeek::Type* type);

/**
 * Convert a Bro threading::Value to a Broker data value.
 * @param v a Bro threading::Value.
 * @return a Broker data value if the Bro threading::Value could be converted to one.
 */
broker::expected<broker::data> threading_val_to_data(const threading::Value* v);

/**
 * Convert a Bro threading::Field to a Broker data value.
 * @param f a Bro threading::Field.
 * @return a Broker data value if the Bro threading::Field could be converted to one.
 */
broker::data threading_field_to_data(const threading::Field* f);

/**
 * Convert a Broker data value to a Bro threading::Value.
 * @param d a Broker data value.
 * @return a pointer to a new Bro threading::Value or a nullptr if the conversion was not
 * possible.
 */
threading::Value* data_to_threading_val(broker::data d);

/**
 * Convert a Broker data value to a Bro threading::Value.
 * @param d a Broker data value.
 * @return a pointer to a new Bro threading::Value or a nullptr if the conversion was not
 * possible.
 */
threading::Field* data_to_threading_field(broker::data d);

/**
 * A Bro value which wraps a Broker data value.
 */
class DataVal : public OpaqueVal {
public:

	DataVal(broker::data arg_data)
		: OpaqueVal(bro_broker::opaque_of_data_type), data(std::move(arg_data))
		{}

	void ValDescribe(ODesc* d) const override;

	IntrusivePtr<Val> castTo(zeek::Type* t);
	bool canCastTo(zeek::Type* t) const;

	// Returns the Bro type that scripts use to represent a Broker data
	// instance. This may be wrapping the opaque value inside another
	// type.
	static const IntrusivePtr<zeek::Type>& ScriptDataType();

	broker::data data;

protected:
	DataVal()
		: OpaqueVal(bro_broker::opaque_of_data_type)
		{}

	DECLARE_OPAQUE_VALUE(bro_broker::DataVal)
};

/**
 * Visitor for retrieving type names a Broker data value.
 */
struct type_name_getter {
	using result_type = const char*;

	result_type operator()(broker::none)
		{ return "NONE"; } // FIXME: what's the right thing to return here?

	result_type operator()(bool)
		{ return "bool"; }

	result_type operator()(uint64_t)
		{ return "uint64_t"; }

	result_type operator()(int64_t)
		{ return "int64_t"; }

	result_type operator()(double)
		{ return "double"; }

	result_type operator()(const std::string&)
		{ return "string"; }

	result_type operator()(const broker::address&)
		{ return "address"; }

	result_type operator()(const broker::subnet&)
		{ return "subnet"; }

	result_type operator()(const broker::port&)
		{ return "port"; }

	result_type operator()(const broker::timestamp&)
		{ return "time"; }

	result_type operator()(const broker::timespan&)
		{ return "interval"; }

	result_type operator()(const broker::enum_value&)
		{ return "enum"; }

	result_type operator()(const broker::set&)
		{ return "set"; }

	result_type operator()(const broker::table&)
		{ return "table"; }

	result_type operator()(const broker::vector&)
		{
		assert(tag == zeek::TYPE_VECTOR || tag == zeek::TYPE_RECORD);
		return tag == zeek::TYPE_VECTOR ? "vector" : "record";
		}

	zeek::TypeTag tag;
};

/**
 * Retrieve Broker data value associated with a Broker::Data Bro value.
 * @param v a Broker::Data value.
 * @param f used to get location information on error.
 * @return a reference to the wrapped Broker data value.  A runtime interpreter
 * exception is thrown if the the optional opaque value of \a v is not set.
 */
broker::data& opaque_field_to_data(RecordVal* v, Frame* f);

/**
 * Retrieve variant data from a Broker data value.
 * @tparam T a type that the variant may contain.
 * @param d a Broker data value to get variant data out of.
 * @param tag a Bro tag which corresponds to T (just used for error reporting).
 * @param f used to get location information on error.
 * @return a refrence to the requested type in the variant Broker data.
 * A runtime interpret exception is thrown if trying to access a type which
 * is not currently stored in the Broker data.
 */
template <typename T>
T& require_data_type(broker::data& d, zeek::TypeTag tag, Frame* f)
	{
	auto ptr = caf::get_if<T>(&d);
	if ( ! ptr )
		reporter->RuntimeError(f->GetCall()->GetLocationInfo(),
		                       "data is of type '%s' not of type '%s'",
		                       caf::visit(type_name_getter{tag}, d),
		                       zeek::type_name(tag));

	return *ptr;
	}

/**
 * @see require_data_type() and opaque_field_to_data().
 */
template <typename T>
inline T& require_data_type(RecordVal* v, zeek::TypeTag tag, Frame* f)
	{
	return require_data_type<T>(opaque_field_to_data(v, f), tag, f);
	}

// Copying data in to iterator vals is not the fastest approach, but safer...

class SetIterator : public OpaqueVal {
public:

	SetIterator(RecordVal* v, zeek::TypeTag tag, Frame* f)
	    : OpaqueVal(bro_broker::opaque_of_set_iterator),
	      dat(require_data_type<broker::set>(v, zeek::TYPE_TABLE, f)),
	      it(dat.begin())
		{}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	[[deprecated("Remove in v4.1. Use the version that takes zeek::TypeTag instead.")]]
	SetIterator(RecordVal* v, ::TypeTag tag, Frame* f)
		: SetIterator(v, static_cast<zeek::TypeTag>(tag), f)
		{}
#pragma GCC diagnostic pop

	broker::set dat;
	broker::set::iterator it;

protected:
	SetIterator()
		: OpaqueVal(bro_broker::opaque_of_set_iterator)
		{}

	DECLARE_OPAQUE_VALUE(bro_broker::SetIterator)
};

class TableIterator : public OpaqueVal {
public:

	TableIterator(RecordVal* v, zeek::TypeTag tag, Frame* f)
	    : OpaqueVal(bro_broker::opaque_of_table_iterator),
	      dat(require_data_type<broker::table>(v, zeek::TYPE_TABLE, f)),
	      it(dat.begin())
		{}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	[[deprecated("Remove in v4.1. Use the version that takes zeek::TypeTag instead.")]]
	TableIterator(RecordVal* v, ::TypeTag tag, Frame* f)
		: TableIterator(v, static_cast<zeek::TypeTag>(tag), f)
		{}
#pragma GCC diagnostic pop

	broker::table dat;
	broker::table::iterator it;

protected:
	TableIterator()
		: OpaqueVal(bro_broker::opaque_of_table_iterator)
		{}

	DECLARE_OPAQUE_VALUE(bro_broker::TableIterator)
};

class VectorIterator : public OpaqueVal {
public:

	VectorIterator(RecordVal* v, zeek::TypeTag tag, Frame* f)
	    : OpaqueVal(bro_broker::opaque_of_vector_iterator),
	      dat(require_data_type<broker::vector>(v, zeek::TYPE_VECTOR, f)),
	      it(dat.begin())
		{}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	[[deprecated("Remove in v4.1. Use the version that takes zeek::TypeTag instead.")]]
	VectorIterator(RecordVal* v, ::TypeTag tag, Frame* f)
		: VectorIterator(v, static_cast<zeek::TypeTag>(tag), f)
		{}
#pragma GCC diagnostic pop

	broker::vector dat;
	broker::vector::iterator it;

protected:
	VectorIterator()
		: OpaqueVal(bro_broker::opaque_of_vector_iterator)
		{}

	DECLARE_OPAQUE_VALUE(bro_broker::VectorIterator)
};

class RecordIterator : public OpaqueVal {
public:

	RecordIterator(RecordVal* v, zeek::TypeTag tag, Frame* f)
	    : OpaqueVal(bro_broker::opaque_of_record_iterator),
	      dat(require_data_type<broker::vector>(v, zeek::TYPE_RECORD, f)),
	      it(dat.begin())
		{}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	[[deprecated("Remove in v4.1. Use the version that takes zeek::TypeTag instead.")]]
	RecordIterator(RecordVal* v, ::TypeTag tag, Frame* f)
		: RecordIterator(v, static_cast<zeek::TypeTag>(tag), f)
		{}
#pragma GCC diagnostic pop

	broker::vector dat;
	broker::vector::iterator it;

protected:
	RecordIterator()
		: OpaqueVal(bro_broker::opaque_of_record_iterator)
		{}

	DECLARE_OPAQUE_VALUE(bro_broker::RecordIterator)
};

} // namespace bro_broker
