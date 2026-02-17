#ifndef SLZ_VRS_PLUGIN_NO_API
#define SLZ_VRS_PLUGIN_NO_API

#include <string>
#include "PluginAPI.h"

class PluginAPINone : public PluginAPI
{
public:
	std::string GetStatusMessage() { return std::string("SLZ VRS not intialized. Graphics API is not compatible"); }
};

PluginAPI* CreateApiUnknown()
{
	return new PluginAPINone;
}
#endif
