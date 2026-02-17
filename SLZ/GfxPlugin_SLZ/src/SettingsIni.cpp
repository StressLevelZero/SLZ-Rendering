#include "SettingsIni.h"
#include "Platform.h"
#include <fstream>
#include <format>
#include "inipp/inipp/inipp.h"

typedef enum SettingsType
{
    SETTING_INT = 0,
    SETTING_FLOAT = 1 
} SettingsType;

struct IntSetting
{
    const char* name;
    const int defaultVal;
};

struct FloatSetting
{
    const char* name;
    const int defaultVal;
};

#if defined(PLATFORM_PC)

#define DEFAULT_ALLOW_PIPELINE_RATE_HACK	0
#define DEFAULT_MAX_ANISO_SAMPLES			-1
#define DEFAULT_DISABLE_SAMPLER_HOOK		0

#else

#define DEFAULT_ALLOW_PIPELINE_RATE_HACK	1
#define DEFAULT_MAX_ANISO_SAMPLES			4
#define DEFAULT_DISABLE_SAMPLER_HOOK		0

#endif

constexpr IntSetting intSettingDefinitions[] =
{
    {.name = "allow_pipeline_rate_hack",	.defaultVal = DEFAULT_ALLOW_PIPELINE_RATE_HACK	},
    {.name = "max_aniso_samples",			.defaultVal = DEFAULT_MAX_ANISO_SAMPLES			},
    {.name = "disable_sampler_hook",		.defaultVal = DEFAULT_DISABLE_SAMPLER_HOOK		}
};

constexpr FloatSetting floatSettingDefinitions[1] =
{

};

SLZGraphicsConfig::SLZGraphicsConfig()
{
    constexpr int intSettingsCount = std::size(intSettingDefinitions);
    constexpr int floatSettingsCount = 0;// std::size(floatSettingDefinitions); // uncomment this when you actually add a float setting

    if (intSettingsCount > 0) this->intSettings = std::unique_ptr<int[]>(new int[intSettingsCount]);
    if (floatSettingsCount > 0) this->floatSettings = std::unique_ptr<float[]>(new float[floatSettingsCount]);

    this->settingNameToIdx = std::unordered_map<std::string, settingIdx>(intSettingsCount + floatSettingsCount);

    for (int intIdx = 0; intIdx < intSettingsCount; intIdx++)
    {
        this->settingNameToIdx.emplace(
                std::string(intSettingDefinitions[intIdx].name),
                settingIdx{ .type = SETTING_INT, .idx = intIdx }
        );
        this->intSettings[intIdx] = intSettingDefinitions[intIdx].defaultVal;
    }
    for (int fltIdx = 0; fltIdx < floatSettingsCount; fltIdx++)
    {
        this->settingNameToIdx.emplace(
                std::string(floatSettingDefinitions[fltIdx].name),
                settingIdx{ .type = SETTING_INT, .idx = fltIdx }
        );
        this->intSettings[fltIdx] = floatSettingDefinitions[fltIdx].defaultVal;
    }


}

std::string SLZGraphicsConfig::PrintSettings()
{
    std::string message = "Current native settings:\n";
    int intSettingsCount = std::size(intSettingDefinitions);
    for (int intIdx = 0; intIdx < intSettingsCount; intIdx++)
    {
        message += std::format("{}: {}\n", intSettingDefinitions[intIdx].name, intSettings[intIdx]);
    }
    return message;
}


inline void Trim(char* line, int stringLen, int* beginIdx, int* length)
{
    int idx = 0;
    while (idx < stringLen && (line[idx] == ' ' || line[idx] == '\t'))
    {
        idx += 1;
    }
    *beginIdx = idx;
    int headerBeginIdx = idx;
    idx = stringLen - 1;
    while (idx > headerBeginIdx && (line[idx] == ' ' || line[idx] == '\t'))
    {
        idx -= 1;
    }
    *length = idx - headerBeginIdx + 1;
}

int SLZGraphicsConfig::ReadSettingsIni(std::ifstream* file)
{
    inipp::Ini<char> ini;
    ini.parse(*file);
    if (!ini.sections.contains("native"))
    { 
        return 1;
    }
    constexpr int numIntSettings = std::size(intSettingDefinitions);
    auto nativeMap = ini.sections["native"];
    for (int intIdx = 0; intIdx < numIntSettings; intIdx++)
    {
        int value = -1;
        if (inipp::get_value(nativeMap, intSettingDefinitions[intIdx].name, value))
        {
            intSettings[intIdx] = value;
        }
    }

    this->allow2x2ShadingHack = intSettings[settingNameToIdx["allow_pipeline_rate_hack"].idx];
    this->maxAnisotropy = intSettings[settingNameToIdx["max_aniso_samples"].idx];
    this->disableSamplerHook = intSettings[settingNameToIdx["disable_sampler_hook"].idx];

    return 0;
}

