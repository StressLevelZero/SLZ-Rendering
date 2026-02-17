#if !defined(SLZ_GRAPHICS_CONFIG)
#define SLZ_GRAPHICS_CONFIG
#include "Platform.h"
#include <unordered_map>
#include <vector>

class SLZGraphicsConfig
{
public:
	static SLZGraphicsConfig* s_Instance;
	static inline int maxAnisotropy = 16.0f;
	static inline bool allow2x2ShadingHack = true;
	static inline int disableSamplerHook = 0;

	SLZGraphicsConfig();
	int ReadSettingsIni(std::ifstream* file);
	std::string PrintSettings();
private:
	typedef struct settingIdx
	{
		int type;
		int idx;
	} settingIdx;

	std::unordered_map<std::string, settingIdx> settingNameToIdx;
	std::unique_ptr<int[]> intSettings;
	std::unique_ptr<float[]> floatSettings;
};
#endif
