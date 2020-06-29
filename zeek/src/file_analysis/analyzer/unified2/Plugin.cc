// See the file  in the main distribution directory for copyright.

// See the file "COPYING" in the main distribution directory for copyright.

#include "Unified2.h"
#include "plugin/Plugin.h"
#include "file_analysis/Component.h"

namespace plugin {
namespace Zeek_Unified2 {

class Plugin : public zeek::plugin::Plugin {
public:
	zeek::plugin::Configuration Configure() override
		{
		AddComponent(new ::file_analysis::Component("UNIFIED2", ::file_analysis::Unified2::Instantiate));

		zeek::plugin::Configuration config;
		config.name = "Zeek::Unified2";
		config.description = "Analyze Unified2 alert files.";
		return config;
		}
} plugin;

}
}
