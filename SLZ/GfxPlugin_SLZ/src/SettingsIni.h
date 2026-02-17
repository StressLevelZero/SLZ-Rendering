#pragma once

#include "Platform.h"
#include <unordered_map>
#include <vector>

class SLZGraphicsConfig
{
public:
	static inline SLZGraphicsConfig* s_Instance = nullptr;
	int maxAnisotropy = 16;
	bool disableNVExtensions = 0;
	bool allow2x2ShadingHack = 0;
	int disableSamplerHook = 0;

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