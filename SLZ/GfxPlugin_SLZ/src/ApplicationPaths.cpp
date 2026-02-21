// Needs to be defined before including dlfcn.h for dladdr to exist
#if defined(__linux__)
#define _GNU_SOURCE 1
#endif

#include "Platform.h"
#include "DebugLog.h"

#include "PluginState.h"
#include "ApplicationPaths.h"

#include <filesystem>
#include <fstream>

#if defined(PLATFORM_WINDOWS)
    #include <cstdlib>
    #include <stringapiset.h>
    #include <KnownFolders.h>
#endif

#if defined(PLATFORM_ANDROID)
    #include "Android/JNIinterface.hpp"
#endif

using namespace std;
namespace fs = std::filesystem;



class AppPathsPrivate
{
public:
    static bool DetermineIfEditor(fs::path libraryPath);
    static fs::path RuntimeGetVendorProductPath(const fs::path libraryPath, int& errorCode);
    static fs::path EditorGetVendorProductPath(const fs::path libraryPath, int& errorCode);
    static fs::path GetRootPersistentDataPath(int& errorCode);
#if !defined(PLATFORM_WINDOWS)
    static fs::path GetLibAddress(int& errorCode);
#endif
};


int ApplicationPaths::PopulatePathsAndEditor()
{
#if defined(PLATFORM_ANDROID)

    ApplicationPaths::isEditor = false; // There's no android editor
    ApplicationPaths::peristentDataPath = s_jni->externalFilesDir;
    return s_jni->externalFilesDir.empty() ? 0 : 1;
#else  // NOT PLATFORM_ANDROID

    int errorCode = 0;

#if defined(PLATFORM_WINDOWS) // Library path will be populated on module load on windows

    if (libraryPath.empty())
    {
        PluginState::Log(kUnityLogTypeException, "Shared library path was not populated on module load!", __FILE__, __LINE__);
        return 1;
    }
#else // Linux
    libraryPath = AppPathsPrivate::GetLibAddress(errorCode);
    if (errorCode != 0)
    {
        PluginState::Log(kUnityLogTypeException, "Failed to get shared library path!", __FILE__, __LINE__);
        return 1;
    }
#endif

    ApplicationPaths::isEditor = AppPathsPrivate::DetermineIfEditor(libraryPath);
    fs::path rootPersistentPath = AppPathsPrivate::GetRootPersistentDataPath(errorCode);
    if (errorCode != 0)
    {
        PluginState::Log(kUnityLogTypeException, "Failed to get root persistent data path!", __FILE__, __LINE__);
        return 2;
    }
    fs::path vendorProduct;
    if (ApplicationPaths::isEditor)
    {
        vendorProduct = AppPathsPrivate::EditorGetVendorProductPath(libraryPath, errorCode);
    }
    else
    {
        vendorProduct = AppPathsPrivate::RuntimeGetVendorProductPath(libraryPath, errorCode);
    }
    if (errorCode != 0)
    {
        PluginState::Log(kUnityLogTypeException, "Failed to get app's vendor/product for finding the persistent data path!", __FILE__, __LINE__);
        return 3;
    }
    ApplicationPaths::peristentDataPath = rootPersistentPath / vendorProduct;
    return 0;
#endif // NOT PLATFORM_ANDROID
}


// Shitty but reliable way of determining if we're in editor. There will always be a meta file for this shared library if this is running in editor
bool AppPathsPrivate::DetermineIfEditor(fs::path libraryPath)
{
    fs::path metaPath(libraryPath);
    metaPath += ".meta";
    bool isEditor = fs::exists(metaPath);
    return isEditor;
}

// Stupid way of determining the vendor name and product name needed for finding the persistent data path. Unity conveniently puts them into a text file with the game files,
// and we can find it by navigating upward from this shared library's path in the plugins folder.
fs::path AppPathsPrivate::RuntimeGetVendorProductPath(const fs::path libraryPath, int& errorCode)
{
    fs::path gameDataPath = libraryPath.parent_path().parent_path().parent_path(); // in builds, this dll is in GAME/GAME_data/plugins/x86_64/pluginName.dll, we want GAME/GAME_data
    fs::path appinfoPath = gameDataPath / "app.info";

    if (!fs::exists(appinfoPath))
    {
        errorCode = 1;
        return fs::path("");
    }

    bool foundAppInfo = false;
    std::ifstream appInfoFile(appinfoPath, std::ios_base::in WINDOWS_OPEN_FILE_SHARED);
    if (appInfoFile.fail())
    {
        //PluginState::Log(kUnityLogTypeError, std::format("Failed to open app.info file at {}", appinfoPath.u8string().c_str()).c_str(), __FILE__, __LINE__);
        errorCode = 1;
        return fs::path("");
    }

    std::string appinfoU8_vendor;
    std::string appinfoU8_product;
    std::getline(appInfoFile, appinfoU8_vendor);
    std::getline(appInfoFile, appinfoU8_product);
    if (appinfoU8_vendor.empty() || appinfoU8_product.empty())
    {
        //PluginState::Log(kUnityLogTypeError, std::format("Failed to read 2 lines from app.info, possibly empty? {}", appinfoPath.u8string().c_str()).c_str(), __FILE__, __LINE__);
        errorCode = 2;
        return fs::path("");
    }

    errorCode = 0;
    fs::path vendorProduct = fs::path(appinfoU8_vendor) / fs::path(appinfoU8_product);
    return vendorProduct;

    return fs::path("");
}


fs::path AppPathsPrivate::EditorGetVendorProductPath(const fs::path libraryPath, int& errorCode)
{
    fs::path walk = libraryPath.parent_path();
    fs::path libraryFolder("Library");
    fs::path packagesFolder("Packages");
    fs::path assetsFolder("Assets");
    fs::path SLZRenderingFolder("SLZ-Rendering");
    fs::path projectPath;
    while (!walk.empty())
    {
        fs::path foldername = walk.filename();
        if (foldername == libraryFolder || foldername == packagesFolder || foldername == assetsFolder)
        {
            
            projectPath = walk.parent_path();
            if (projectPath.filename() == SLZRenderingFolder)
            {
                projectPath = projectPath.parent_path();
            }
            break;
        }
        walk = walk.parent_path();
    }
    if (projectPath.empty())
    {
        errorCode = 1;
        return fs::path();
    }

    fs::path settingsPath = projectPath / "ProjectSettings" / "ProjectSettings.asset";
    std::ifstream projectSettingsFile(settingsPath, std::ios_base::in WINDOWS_OPEN_FILE_SHARED);
    if (projectSettingsFile.fail())
    {
        PluginState::Log(kUnityLogTypeError, std::format("Failed to open ProjectSettings file at {}", (char*)(settingsPath.u8string().c_str())).c_str(), __FILE__, __LINE__);
        errorCode = 1;
        return fs::path();
    }

    bool foundVendor = false;
    std::string vendor;
    bool foundProduct = false;
    std::string product;

    constexpr char yamlCompanyName[] = "  companyName: ";
    constexpr int yamlCompanyNameLen = std::size(yamlCompanyName) - 1;
    constexpr char yamlProductName[] = "  productName: ";
    constexpr int yamlProductNameLen = std::size(yamlProductName) - 1;

    for (std::string line; std::getline(projectSettingsFile, line); )
    {
        if (!foundVendor)
        {
            bool isVendor = line.starts_with(yamlCompanyName);
            if (isVendor && line.length() > yamlCompanyNameLen)
            {
                vendor = line.substr(yamlCompanyNameLen, line.length() - yamlCompanyNameLen);
                foundVendor = true;
            }
        }
        if (!foundProduct)
        {
            int isProduct = line.starts_with(yamlProductName);
            if (isProduct && line.length() > yamlProductNameLen)
            {
                product = line.substr(yamlProductNameLen, line.length() - yamlProductNameLen);
                foundProduct = true;
            }
        }
        if (foundVendor && foundProduct)
        {
            break;
        }
    }

    if (!foundVendor && !foundProduct)
    {
        PluginState::Log(kUnityLogTypeError, std::format("Failed to find company and product in ProjectSettings file at {}", (char*)(settingsPath.u8string().c_str())).c_str(), __FILE__, __LINE__);
        errorCode = 1;
        return fs::path();
    }

    fs::path vendorProduct = std::filesystem::path(vendor) / std::filesystem::path(product);
    errorCode = 0;

    return vendorProduct;
}





#if defined(PLATFORM_WINDOWS)

fs::path ApplicationPaths::WinGetLibAddress(HMODULE dllModule, int& errorCode)
{
    DWORD bufferSize = 256;
    std::unique_ptr<PATH_CHAR[]> pathBuff;
    DWORD pathLen = 0;
    do
    {
        bufferSize *= 2;
        pathBuff = std::make_unique<PATH_CHAR[]>(bufferSize + 1);
        pathLen = GetModuleFileNameW(dllModule, pathBuff.get(), bufferSize);
    } while (GetLastError() == ERROR_INSUFFICIENT_BUFFER && bufferSize <= 0x10000);

    errorCode = GetLastError();

    if (errorCode != ERROR_SUCCESS)
    {
        return fs::path("");
    }

    pathBuff[pathLen] = PATH_LITERAL('\0');
    fs::path pathStr(pathBuff.get());
    return pathStr;
}


typedef HRESULT WINAPI pfn_SHGetKnownFolderPath(REFGUID rfid, DWORD dwFlags, HANDLE hToken, PWSTR* ppszPath);

fs::path AppPathsPrivate::GetRootPersistentDataPath(int& errorCode)
{
    HMODULE libShell32 = LoadLibraryW(L"shell32.dll");

    if (!libShell32)
    {
        errorCode = 1;
        return fs::path("");
    }

    pfn_SHGetKnownFolderPath* shGetKnownFolderPath = reinterpret_cast<pfn_SHGetKnownFolderPath*>(GetProcAddress(libShell32, "SHGetKnownFolderPath"));
    if (!shGetKnownFolderPath)
    {
        errorCode = 2;
        return fs::path("");
    }

    HRESULT pathError = S_FALSE;
    wchar_t* outPath = NULL;
    pathError = shGetKnownFolderPath(FOLDERID_LocalAppDataLow, NULL, NULL, &outPath);

    if (pathError != S_OK)
    {
        errorCode = 3;
        return fs::path("");
    }


    FreeLibrary(libShell32);

    fs::path rootPersistentPath(outPath);
    bool exists = fs::exists(rootPersistentPath);
    errorCode = exists ? 0 : 4;

    return rootPersistentPath;
}

#elif defined(PLATFORM_LINUX)

// Exported dummy function for linux's dladdr to find the address of 
extern "C" int DLLEXPORT STDCALL DummyFunc() { return 0; }

fs::path AppPathsPrivate::GetLibAddress(int& errorCode)
{
    Dl_info dlinfo = {};

    void* DummyFuncPtr = (void*)DummyFunc;
    errorCode = dladdr(DummyFuncPtr, &dlinfo);

    if (errorCode == 0)
    {
        PluginState::Log(kUnityLogTypeError, std::format("Failed to find library address, function address is {0:#x}, dlinfo path is : {1}", (unsigned long)DummyFuncPtr, (const char*)dlinfo.dli_fname).c_str(), __FILE__, __LINE__);

        return fs::path("");
    }

    fs::path pathStr((const char8_t*)dlinfo.dli_fname);
    return pathStr;
}

fs::path AppPathsPrivate::GetRootPersistentDataPath(int& errorCode)
{
    const char* xdgConfigHome = std::getenv("XDG_CONFIG_HOME");
    if (xdgConfigHome == nullptr)
    {
        xdgConfigHome = "~/.config";
    }
    fs::path rootPersistentPath((const char8_t*)xdgConfigHome);
    rootPersistentPath /= u8"unity3d";

    bool exists = fs::exists(rootPersistentPath);
    if (!exists)
    {
        PluginState::Log(kUnityLogTypeError, std::format("Config home directory does not exist?: {}", (char*)(rootPersistentPath.u8string().c_str())).c_str(), __FILE__, __LINE__);
    }
    errorCode = exists ? 0 : 1;
    return rootPersistentPath;
}

#endif

