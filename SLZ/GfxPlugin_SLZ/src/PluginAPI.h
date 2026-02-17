#ifndef SLZ_VRS_PLUGIN_API
#define SLZ_VRS_PLUGIN_API

#include <string>


class PluginAPI
{
public: 
    virtual void GfxEventInit() {};
    virtual void GfxEventShutdown() {};
	virtual std::string GetStatusMessage() = 0;

    virtual void OnRenderEventWithData(int eventID, void* data) {};
    virtual void OnRenderEvent(int eventID) {};

};

#endif
