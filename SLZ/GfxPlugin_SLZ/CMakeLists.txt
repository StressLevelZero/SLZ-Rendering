cmake_minimum_required(VERSION 3.22)

if(${CMAKE_VERSION} VERSION_LESS 3.5)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
else()
    cmake_policy(VERSION 3.22)
endif()

project(GfxPlugin_SLZ
        VERSION 1.0.0
        DESCRIPTION "Stress Level Zero's Unity native rendering plugin"
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# Options
option(PRINT_HELP "Print help message" ON)
set(UNITY_PLUGIN_HEADERS "" CACHE PATH "Path to unity's PluginAPI headers. Found in (unity editor directory)/(version number)/Editor/Data/")
#set(BUILD_TARGET "WINDOWS" CACHE STRING "Target OS to build for (WINDOWS | ANDROID | LINUX)")

# Help message, shove in the user's face so they know they need to set required arguments
if (PRINT_HELP)
    message(STATUS "Usage:\
\n-DPRINT_HELP:BOOL            Print this help message, default ON\
\n-DUNITY_PLUGIN_HEADERS:PATH  Path to Unity Editor's PluginAPI folder\
\n\nExample usage:\
\ncmake -DPRINT_HELP=OFF -DUNITY_PLUGIN_HEADERS=\"C:\\Unity\\1234.5.6f1\\Editor\\Data\\PluginAPI\" -G \"Visual Studio 17 2022\" -T ClangCl -DCMAKE_BUILD_TYPE=Release
    ")
    message(FATAL_ERROR "Exiting, set -DPRINT_HELP=OFF to build")
endif()

# Check unity plugin api directory to make sure it's valid
if (NOT IS_DIRECTORY "${UNITY_PLUGIN_HEADERS}")
    message(FATAL_ERROR "UNITY_PLUGIN_HEADERS not set to a valid path (${UNITY_PLUGIN_HEADERS})")
endif()

if (NOT EXISTS  "${UNITY_PLUGIN_HEADERS}/IUnityInterface.h")
    message(FATAL_ERROR "IUnityInterface.h not found in UNITY_PLUGIN_HEADERS (${UNITY_PLUGIN_HEADERS})")
endif()

# Import the vulkan SDK
find_package(Vulkan REQUIRED )



# Set build target specific strings
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(TARGET_PREFIX "win")
    string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" TARGET_ARCH)
    set(STATIC_LIB_EXT "lib")
    set(STATIC_LIB_PREFIX "")
    set(VS_WINDOWS_TARGET_PLATFORM_MIN_VERSION 10.0.22621.0)
    set(CMAKE_VS_WINDOWS_TARGET_PLATFORM_MIN_VERSION 10.0.22621.0)
    if (VS_WINDOWS_TARGET_PLATFORM_MIN_VERSION VERSION_LESS 10.0.22621.0)
        set(VS_WINDOWS_TARGET_PLATFORM_MIN_VERSION 10.0.22621.0)
    endif()
set(CMAKE_CXX_STANDARD 26)
    
elseif(CMAKE_SYSTEM_NAME STREQUAL "Android")
    set(TARGET_PREFIX "android")
    set(TARGET_ARCH "${ANDROID_ABI}")
    set(STATIC_LIB_EXT "a")
    set(STATIC_LIB_PREFIX "lib")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(TARGET_PREFIX "linux")
    string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" TARGET_ARCH)
    set(STATIC_LIB_EXT "a")
    set(STATIC_LIB_PREFIX "lib")
    set( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -fPIC")
else()
    message(FATAL_ERROR "Building for unknown operating system ${CMAKE_SYSTEM_NAME}, you will need to modify this script to handle it!")
endif()


set(SRC_PATH "${CMAKE_SOURCE_DIR}/src")
include_directories("${SRC_PATH}")

set(EXTERN_PATH "${CMAKE_SOURCE_DIR}/extern")
include_directories("${EXTERN_PATH}")

add_library(UnityPluginAPI INTERFACE)
target_include_directories(UnityPluginAPI INTERFACE "${UNITY_PLUGIN_HEADERS}")
set_target_properties(UnityPluginAPI PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${UNITY_PLUGIN_HEADERS}")

add_library(GraphicsConfig STATIC "${SRC_PATH}/SLZGraphicsConfig.cpp")
target_link_libraries(GraphicsConfig PRIVATE UnityPluginAPI)

add_library(ApplicationPaths STATIC
    "${SRC_PATH}/ApplicationPaths.cpp"
)
target_link_libraries(ApplicationPaths PRIVATE UnityPluginAPI)

add_library(GfxPlugin_SLZ SHARED 
    "${SRC_PATH}/Main.cpp" 
    "${SRC_PATH}/PluginApiVk.cpp" 
    )
    
target_link_libraries(GfxPlugin_SLZ PUBLIC GraphicsConfig ApplicationPaths UnityPluginAPI)
# target_link_libraries(GfxPlugin_SLZ PUBLIC Vulkan::Headers)
target_include_directories(GfxPlugin_SLZ PUBLIC "${Vulkan_INCLUDE_DIR}")

if (CMAKE_SYSTEM_NAME STREQUAL "Android")

add_library(JniInterface STATIC
    "${SRC_PATH}/Android/JNIInterface.cpp"
    )

target_link_libraries(ApplicationPaths PUBLIC JniInterface)

target_link_libraries(ApplicationPaths INTERFACE android log)
target_link_libraries(GfxPlugin_SLZ    INTERFACE android log)

endif()
