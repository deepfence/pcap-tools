// See the file "COPYING" in the main distribution directory for copyright.

#pragma once

#include "plugin/Component.h"

#include <string>
#include <vector>

namespace iosource {

class IOSource;
class PktSrc;
class PktDumper;

/**
 * Component description for plugins providing IOSources.
 */
class Component : public zeek::plugin::Component {
public:
	typedef IOSource* (*factory_callback)();

	/**
	 * Constructor.
	 *
	 * @param name A descriptive name for the component.  This name must
	 * be unique across all components of this type.
	 */
	explicit Component(const std::string& name);

	/**
	 * Destructor.
	 */
	~Component() override;

protected:

	/**
	 * Constructor to use by derived classes.
	 *
	 * @param type The type of the componnent.
	 *
	 * @param name A descriptive name for the component.  This name must
	 * be unique across all components of this type.
	 */
	Component(zeek::plugin::component::Type type, const std::string& name);

	/**
	 * Constructor to use by derived classes.
	 *
	 * @param type The type of the componnent.
	 *
	 * @param name A descriptive name for the component.  This name must
	 * be unique across all components of this type.
	 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	[[deprecated("Remove in v4.1. Use the version that takes zeek::plugin::component::Type instead")]]
	Component(plugin::component::Type type, const std::string& name);
#pragma GCC diagnostic pop
};

/**
 * Component description for plugins providing a PktSrc for packet input.
 */
class PktSrcComponent : public iosource::Component {
public:
	/**
	 * Type of input a packet source supports.
	 */
	enum InputType {
		LIVE,	///< Live input.
		TRACE,		///< Offline input from trace file.
		BOTH	///< Live input as well as offline.
	};

	typedef PktSrc* (*factory_callback)(const std::string& path, bool is_live);

	/**
	 * Constructor.
	 *
	 * @param name A descriptive name for the component.  This name must
	 * be unique across all components of this type.
	 *
	 * @param prefixes The list of interface/file prefixes associated
	 * with this component.
	 *
	 * @param type Type of input the component supports.
	 *
	 * @param factor Factory function to instantiate component.
	 */
	PktSrcComponent(const std::string& name, const std::string& prefixes, InputType type, factory_callback factory);

	/**
	 * Destructor.
	 */
	~PktSrcComponent() override;

	/**
	 * Returns the prefix(es) passed to the constructor.
	 */
	const std::vector<std::string>& Prefixes() const;

	/**
	 * Returns true if the given prefix is among the one specified for the component.
	 */
	bool HandlesPrefix(const std::string& prefix) const;

	/**
	 * Returns true if packet source instantiated by the component handle
	 * live traffic.
	 */
	bool DoesLive() const;

	/**
	 * Returns true if packet source instantiated by the component handle
	 * offline traces.
	 */
	bool DoesTrace() const;

	/**
	 * Returns the source's factory function.
	 */
	factory_callback Factory() const;

	/**
	 * Generates a human-readable description of the component. This goes
	 * into the output of \c "bro -NN".
	 */
	void DoDescribe(ODesc* d) const override;

private:
	std::vector<std::string> prefixes;
	InputType type;
	factory_callback factory;
};

/**
 * Component description for plugins providing a PktDumper for packet output.
 *
 * PktDumpers aren't IOSurces but we locate them here to keep them along with
 * the PktSrc.
 */
class PktDumperComponent : public zeek::plugin::Component  {
public:
	typedef PktDumper* (*factory_callback)(const std::string& path, bool append);

	/**
	 * XXX
	 */
	PktDumperComponent(const std::string& name, const std::string& prefixes, factory_callback factory);

	/**
	 * Destructor.
	 */
	~PktDumperComponent() override;

	/**
	 * Returns the prefix(es) passed to the constructor.
	 */
	const std::vector<std::string>& Prefixes() const;

	/**
	 * Returns true if the given prefix is among the one specified for the component.
	 */
	bool HandlesPrefix(const std::string& prefix) const;

	/**
	 * Returns the source's factory function.
	 */
	factory_callback Factory() const;

	/**
	 * Generates a human-readable description of the component. This goes
	 * into the output of \c "bro -NN".
	 */
	void DoDescribe(ODesc* d) const override;

private:
	std::vector<std::string> prefixes;
	factory_callback factory;
};

}
