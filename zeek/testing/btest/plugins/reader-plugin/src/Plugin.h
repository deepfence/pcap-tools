
#pragma once

#include <plugin/Plugin.h>

namespace plugin {
namespace Demo_Foo {

class Plugin : public ::plugin::Plugin
{
protected:
	// Overridden from plugin::Plugin.
	virtual plugin::Configuration Configure();
};

extern Plugin plugin;

}
}
