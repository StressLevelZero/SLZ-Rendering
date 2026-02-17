#include "Platform.h"
#include <filesystem>


class ApplicationPaths
{
public:
    static inline std::filesystem::path libraryPath;
    static inline std::filesystem::path peristentDataPath;
    static inline bool isEditor = false;
    static int PopulatePathsAndEditor();
#if defined(PLATFORM_WINDOWS)
    static std::filesystem::path WinGetLibAddress(HMODULE dllModule, int& errorCode);
#endif
};
