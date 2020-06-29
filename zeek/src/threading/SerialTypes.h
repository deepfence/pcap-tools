
#pragma once

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "Type.h"
#include "net_util.h"

class SerializationFormat;

namespace threading {

/**
 * Definition of a log file, i.e., one column of a log stream.
 */
struct Field {
	const char* name;	//! Name of the field.
	//! Needed by input framework. Port fields have two names (one for the
	//! port, one for the type), and this specifies the secondary name.
	const char* secondary_name;
	zeek::TypeTag type;	//! Type of the field.
	zeek::TypeTag subtype;	//! Inner type for sets and vectors.
	bool optional;	//! True if field is optional.

	/**
	 * Constructor.
	 */
	Field(const char* name, const char* secondary_name, zeek::TypeTag type, zeek::TypeTag subtype, bool optional)
		: name(name ? copy_string(name) : nullptr),
		  secondary_name(secondary_name ? copy_string(secondary_name) : nullptr),
		  type(type), subtype(subtype), optional(optional)	{ }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	[[deprecated("Remove in v4.1. Use the version that takes zeek::TypeTag instead")]]
	Field(const char* name, const char* secondary_name, ::TypeTag type, ::TypeTag subtype, bool optional) :
		Field(name, secondary_name, static_cast<zeek::TypeTag>(type), static_cast<zeek::TypeTag>(subtype), optional)
		{}
#pragma GCC diagnostic pop

	/**
	 * Copy constructor.
	 */
	Field(const Field& other)
		: name(other.name ? copy_string(other.name) : nullptr),
		  secondary_name(other.secondary_name ? copy_string(other.secondary_name) : nullptr),
		  type(other.type), subtype(other.subtype), optional(other.optional)	{ }

	~Field()
		{
		delete [] name;
		delete [] secondary_name;
		}

	/**
	 * Unserializes a field.
	 *
	 * @param fmt The serialization format to use. The format handles
	 * low-level I/O.
	 *
	 * @return False if an error occured.
	 */
	bool Read(SerializationFormat* fmt);

	/**
	 * Serializes a field.
	 *
	 * @param fmt The serialization format to use. The format handles
	 * low-level I/O.
	 *
	 * @return False if an error occured.
	 */
	bool Write(SerializationFormat* fmt) const;

	/**
	 * Returns a textual description of the field's type. This method is
	 * thread-safe.
	 */
	std::string TypeName() const;

private:
	// Force usage of constructor above.
	Field()	{}
};

/**
 * Definition of a log value, i.e., a entry logged by a stream.
 *
 * This struct essentialy represents a serialization of a Val instance (for
 * those Vals supported).
 */
struct Value {
	zeek::TypeTag type;	//! The type of the value.
	zeek::TypeTag subtype;	//! Inner type for sets and vectors.
	bool present;	//! False for optional record fields that are not set.

	struct set_t { bro_int_t size; Value** vals; };
	typedef set_t vec_t;
	struct port_t { bro_uint_t port; TransportProto proto; };

	struct addr_t {
		IPFamily family;
		union {
			struct in_addr in4;
			struct in6_addr in6;
		} in;
	};

	// A small note for handling subnet values: Subnet values emitted from
	// the logging framework will always have a length that is based on the
	// internal IPv6 representation (so you have to substract 96 from it to
	// get the correct value for IPv4).
	// However, the Input framework expects the "normal" length for an IPv4
	// address (so do not add 96 to it), because the underlying constructors
	// for the SubNet type want it like this.
	struct subnet_t { addr_t prefix; uint8_t length; };

	/**
	 * This union is a subset of BroValUnion, including only the types we
	 * can log directly. See IsCompatibleType().
	 */
	union _val {
		bro_int_t int_val;
		bro_uint_t uint_val;
		port_t port_val;
		double double_val;
		set_t set_val;
		vec_t vector_val;
		addr_t addr_val;
		subnet_t subnet_val;
		const char* pattern_text_val;

		struct {
			char* data;
			int length;
		} string_val;

		_val() { memset(this, 0, sizeof(_val)); }
	} val;

	/**
	* Constructor.
	*
	* arg_type: The type of the value.
	*
	* arg_present: False if the value represents an optional record field
	* that is not set.
	*/
	Value(zeek::TypeTag arg_type = zeek::TYPE_ERROR, bool arg_present = true)
		: type(arg_type), subtype(zeek::TYPE_VOID), present(arg_present)
		{}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	[[deprecated("Remove in v4.1. Use the version that takes zeek::TypeTag.")]]
	Value(::TypeTag arg_type, bool arg_present = true)
		: Value(static_cast<zeek::TypeTag>(arg_type), arg_present)
		{}
#pragma GCC diagnostic pop

	/**
	* Constructor.
	*
	* arg_type: The type of the value.
	*
	* arg_type: The subtype of the value for sets and vectors.
	*
	* arg_present: False if the value represents an optional record field
	* that is not set.
	 */
	Value(zeek::TypeTag arg_type, zeek::TypeTag arg_subtype, bool arg_present = true)
		: type(arg_type), subtype(arg_subtype), present(arg_present)
		{}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	[[deprecated("Remove in v4.1. Use the version that takes zeek::TypeTag.")]]
	Value(::TypeTag arg_type, ::TypeTag arg_subtype, bool arg_present = true)
		: Value(static_cast<zeek::TypeTag>(arg_type), static_cast<zeek::TypeTag>(arg_subtype), arg_present)
		{}
#pragma GCC diagnostic pop

	/**
	 * Destructor.
	 */
	~Value();

	/**
	 * Unserializes a value.
	 *
	 * @param fmt The serialization format to use. The format handles low-level I/O.
	 *
	 * @return False if an error occured.
	 */
	bool Read(SerializationFormat* fmt);

	/**
	 * Serializes a value.
	 *
	 * @param fmt The serialization format to use. The format handles
	 * low-level I/O.
	 *
	 * @return False if an error occured.
	 */
	bool Write(SerializationFormat* fmt) const;

	/**
	 * Returns true if the type can be represented by a Value. If
	 * `atomic_only` is true, will not permit composite types. This
	 * method is thread-safe. */
	static bool IsCompatibleType(zeek::Type* t, bool atomic_only=false);

	/**
	 * Convenience function to delete an array of value pointers.
	 * @param vals Array of values
	 * @param num_fields Number of members
	 */
	static void delete_value_ptr_array(Value** vals, int num_fields);

	/**
	 * Convert threading::Value to an internal Zeek type, just using the information given in the threading::Value.
	 *
	 * @param source Name of the source of this threading value. This is used for warnings that are raised
	 *               in case an error occurs.
	 * @param val Threading Value to convert to a Zeek Val.
	 * @param have_error Reference to a boolean. This should be set to false when passed in and is set to true
	 *                   in case an error occurs. If this is set to false when the function is called, the function
	 *                   immediately aborts.
	 * @return Val representation of the threading::Value. nullptr on error.
	 */
	static Val* ValueToVal(const std::string& source, const threading::Value* val, bool& have_error);

private:
	friend class ::IPAddr;
	Value(const Value& other) = delete;
};

}
