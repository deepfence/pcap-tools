// See the file  in the main distribution directory for copyright.

#include "TCP.h"
#include "plugin/Plugin.h"
#include "analyzer/Component.h"

namespace plugin {
namespace Zeek_TCP {

class Plugin : public zeek::plugin::Plugin {
public:
	zeek::plugin::Configuration Configure() override
		{
		AddComponent(new ::analyzer::Component("TCP", ::analyzer::tcp::TCP_Analyzer::Instantiate));
		AddComponent(new ::analyzer::Component("TCPStats", ::analyzer::tcp::TCPStats_Analyzer::Instantiate));
		AddComponent(new ::analyzer::Component("CONTENTLINE", nullptr));
		AddComponent(new ::analyzer::Component("Contents", nullptr));

		zeek::plugin::Configuration config;
		config.name = "Zeek::TCP";
		config.description = "TCP analyzer";
		return config;
		}
} plugin;

}
}
