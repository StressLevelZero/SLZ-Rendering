cmake_minimum_required(VERSION 3.25)

if(${CMAKE_VERSION} VERSION_LESS 3.26)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
else()
    cmake_policy(VERSION 3.26)
endif()

project(SLZTextureProcessor 
        VERSION 1.0.0
        DESCRIPTION "Library for processing textures intended for use with the Unity editor"
        LANGUAGES C CXX)        

set(CMAKE_C_STANDARD 11)
# Visual studio doesn't know how to set clang-cl's C standard version, claims its "msvc" but its actually C11 by default
if ((CMAKE_C_COMPILER_ID STREQUAL "Clang") AND (CMAKE_C_SIMULATE_ID STREQUAL "MSVC"))
    
else()
    set(CMAKE_C_STANDARD_REQUIRED TRUE)
endif()
#C++ 11 required for miniexr 
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(compilerFlagsAVX INTERFACE)
add_library(compilerFlagsSSE INTERFACE)
if ((CMAKE_C_COMPILER_ID STREQUAL "Clang") AND (CMAKE_C_SIMULATE_ID STREQUAL "MSVC"))
    set(OpenMP_RUNTIME_MSVC llvm)
    #set( CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} /clang:-std=c17")
    #set( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS}")
    
    # CMake >3.28 tries to set the include flag to -imsvc for Clang-CL, which doesn't work when opening the project in visual studio. 
    #set(CMAKE_INCLUDE_SYSTEM_FLAG_C "/I")
    unset(CMAKE_INCLUDE_SYSTEM_FLAG_C)
    
    target_compile_definitions(compilerFlagsAVX INTERFACE "__F16C__")
    target_compile_options(compilerFlagsAVX
        INTERFACE
        /arch:AVX
        /clang:-mf16c
        /fp:fast
        /clang:-Wno-missing-braces
        )
        
     target_compile_options(compilerFlagsSSE
        INTERFACE
        /clang:-msse4.2
        /fp:fast
        /clang:-Wno-missing-braces
        )
    add_compile_options("$<$<CONFIG:RELEASE>:/clang:-fopenmp;/clang:-fopenmp-simd;/clang:-O3>")
    add_compile_options("$<$<CONFIG:RELWITHDEBINFO>:/clang:-fopenmp;/clang:-fopenmp-simd;/clang:-O3>")
    message(STATUS "Release flags: ${CMAKE_C_FLAGS_RELEASE}")
    string(REPLACE "/O2" "" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
    string(REPLACE "/O2" "" CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELEASE}")
    string(REPLACE "/O2" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
    string(REPLACE "/O2" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELEASE}")
    #add_compile_options("$<CONFIG:RELWITHDEBINFO>:/clang:-O3")
    message(STATUS "Release flags: ${CMAKE_C_FLAGS_RELEASE}")
    message( STATUS "Using Clang-Cl, adding clang -O3 optimization level for release configs. Don't forget to set the optimization level to 'Custom' in Visual Studio to prevent it forcing MSVC \\O2: ${CMAKE_CXX_FLAGS_RELEASE}" )
    message( WARNING "Clang-CL does not (currently) work with OpenMP out of the box. Microsoft decided that their LLVM openmp library is 'not redistributable' so it only installs it with the full visual studio IDE. You'll need to grab libomp.dll from a build of the real LLVM and rename it to libomp.x86_64.dll and put it beside the dll for this library. Otherwise, anyone without VS installed will be unable to load the library!")
elseif(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    meesage(WARNING "MSVC support is untested. Use Clang-Cl if at all possible. The library needs half-precision float conversions, which is supplied by clang/gcc's _Float16 extension. For MSVC I rewrote all the methods to use AVX/F16C vector instructions for half<->float conversions, but I haven't tested them so its likely they're broken.")
    message( WARNING "This library uses Microsoft's currently experimental linkage of MSVC with LLVM's openmp implementation, as full OpenMP >3.0 features are used. As MS considers this experimental, they don't ship the LLVM openmp library with the VS redistributables. You'll need to grab libomp.dll from a windows build of LLVM and rename it to libomp.x86_64.dll and put it beside the dll for this library. Otherwise, anyone without VS installed will be unable to load the library!")
    target_compile_definitions(compilerFlagsAVX INTERFACE "__F16C__")
    target_compile_options(compilerFlagsAVX
        INTERFACE
        /arch:AVX
        /fp:fast
        -openmp:LLVM
        )
elseif(CMAKE_C_COMPILER_ID STREQUAL "Clang")
    set( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -fPIC -Wno-missing-braces -Wno-nan-infinity-disabled -fopenmp -fopenmp-simd -Winline -save-temps")
    set( CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -fPIC -Wno-missing-braces -Wno-nan-infinity-disabled -fopenmp -fopenmp-simd -Winline -save-temps" )
    target_compile_options(compilerFlagsAVX
        INTERFACE
        -mavx
        -mf16c
        -ffast-math
        )
        target_compile_options(compilerFlagsSSE
        INTERFACE
        -msse4.2
        -ffast-math
        )
endif()


set(SRC_PATH "${CMAKE_SOURCE_DIR}/src")
include_directories("${SRC_PATH}")

set(LIB_PATH "${CMAKE_SOURCE_DIR}/lib")
set(EXTERN_PATH "${CMAKE_SOURCE_DIR}/extern")
set(LIB_EXT "lib")
set(SHARED_EXT "dll")


add_subdirectory("${EXTERN_PATH}/cglm/" EXCLUDE_FROM_ALL)


find_package(OpenMP REQUIRED)

set(MANAGED_PATH "${CMAKE_SOURCE_DIR}/netPlugin/SLZTexProc_UnityInterface/")
add_library(Shared STATIC "${SRC_PATH}/Platform.c" "${SRC_PATH}/Platform.h" "${SRC_PATH}/texture.h")
target_include_directories(Shared PUBLIC "${MANAGED_PATH}")
target_link_libraries(Shared INTERFACE cglm_headers)

#set(OPENCL_PATH "${LIB_PATH}/OpenCL-SDK")
#set(OPENCL_INCL_PATH "${OPENCL_PATH}/include")

#list(APPEND CMAKE_PREFIX_PATH "${OPENCL_PATH}")

#find_package(OpenCL REQUIRED NO_CMAKE_SYSTEM_PACKAGE_REGISTRY NO_CMAKE_PACKAGE_REGISTRY)

set(MINIZ_PATH "${LIB_PATH}/miniz")
add_library(miniz STATIC "${MINIZ_PATH}/miniz.c" "${MINIZ_PATH}/miniz.h")
target_link_libraries(miniz PRIVATE compilerFlagsAVX)
target_include_directories(miniz PUBLIC "${MINIZ_PATH}")

set(LIBSPNG_PATH "${LIB_PATH}/libspng")
add_library(libspng INTERFACE)
target_sources(libspng PUBLIC "${LIBSPNG_PATH}/spng.c" "${LIBSPNG_PATH}/spng.h")
target_include_directories(libspng INTERFACE "${LIBSPNG_PATH}")
target_compile_definitions(libspng INTERFACE SPNG_USE_MINIZ SPNG_STATIC)
target_link_libraries(libspng INTERFACE miniz)

set(TINYEXR_PATH "${EXTERN_PATH}/tinyexr")
set(TINYEXR_FP16PATCH_PATH "${LIB_PATH}/tinyexr_fp16patch")
add_library(tinyexr STATIC "${TINYEXR_FP16PATCH_PATH}/tinyexr_fp16.cpp" "${TINYEXR_FP16PATCH_PATH}/tinyexr_fp16.h" "${TINYEXR_PATH}/tinyexr.h")
target_include_directories(tinyexr PUBLIC "${TINYEXR_PATH}" "${TINYEXR_FP16PATCH_PATH}")
target_compile_definitions(tinyexr PRIVATE TINYEXR_USE_MINIZ=1 TINYEXR_USE_OPENMP=1 UNICODE=1)
target_link_libraries(tinyexr PRIVATE miniz OpenMP::OpenMP_CXX compilerFlagsAVX)


add_library(MainOpenMP STATIC 
    "${SRC_PATH}/main_openmp.c"
    "${SRC_PATH}/main_openmp.h"
    )
target_link_libraries(MainOpenMP OpenMP::OpenMP_C Shared compilerFlagsAVX)

add_library(stb_image INTERFACE "${LIB_PATH}/stb/stb_image.h")
target_include_directories(stb_image INTERFACE "${LIB_PATH}/stb/")

add_library(ImageIO STATIC
    "${SRC_PATH}/imageIO.c"
    "${SRC_PATH}/imageIO.h"
    )
#target_sources(ImageIO PUBLIC "${SRC_PATH}/imageIO.c" "${SRC_PATH}/imageIO.h")
target_link_libraries(ImageIO PRIVATE Shared stb_image libspng tinyexr compilerFlagsAVX)


add_library(SLZTextureProcessor SHARED 
    "${SRC_PATH}/main.c" 
    )

target_link_libraries(SLZTextureProcessor PUBLIC
    Shared 
    ImageIO
    MainOpenMP
    compilerFlagsAVX
    )


#add_executable(TestInit "${SRC_PATH}/tests/testInit.c")


## Sail core library names
#set(SAIL_STATIC_LIBS "sail"
#                    "sail-codecs"
#                    "sail-common"
#                    "sail-manip"
#)
#
## list of names of the "extra" shared libraries that Sail uses. 
#set(SAIL_EXTRA_LIBS
#   "aom"
#   "avif"
#   "brotlicommon"
#   "brotlidec"
#   "brotlienc"
#   "gif"
#   "jasper"
#   "jbig"
#   "jpeg"
#   "jxl"
#   "jxl_threads"
#   "png"
#   "tiff"
#   "webp"
#   "webpdecoder"
#   "webpdemux"
#   "z"
#   "zstd"
#   "resvg"
#)
#
## List of names of the object files associated with each shared library in SAIL_EXTRA_LIBS. 
## Unfortunately, sail doesn't copy these to the build directory or bother to rename them to match the shared libraries
#set(SAIL_EXTRA_SHARED_LOCATIONS
#   "aom"
#   "sail-extra-avif"
#   "brotlicommon"
#   "brotlidec"
#   "brotlienc"
#   "gif"
#   "jasper"
#   "jbig"
#   "jpeg"
#   "jxl"
#   "jxl_threads"
#   "libpng16"
#   "tiff"
#   "sail-extra-webp"
#   "sail-extra-webpdecoder"
#   "sail-extra-webpdemux"
#   "zlib"
#   "zstd"
#   "resvg"
#)
#
#add_library(Sail INTERFACE)
#
## Link the core sail libraries to the sail interface. These are expected to be static and
## built with SAIL_COMBINE_CODECS=on
#foreach(LIB IN LISTS SAIL_STATIC_LIBS)
# message(STATUS "Library=${LIB}")
#add_library(Sail-${LIB} STATIC IMPORTED)
#set_target_properties(Sail-${LIB} PROPERTIES
#   IMPORTED_LOCATION "${LIB_PATH}/sail/lib/${LIB}.${LIB_EXT}"
#   INTERFACE_INCLUDE_DIRECTORIES "${LIB_PATH}/sail/include/sail/"
#)
#target_link_libraries(Sail INTERFACE Sail-${LIB})
#endforeach()
#
## Link the "extra" sail libraries to the sail interface. These are always shared (I would really like them to be static...)
#foreach(LIB IN ZIP_LISTS SAIL_EXTRA_LIBS SAIL_EXTRA_SHARED_LOCATIONS)
# message(STATUS "Library=${LIB_0} Shared=${LIB_1}")
#add_library(SailExtra-${LIB_0} SHARED IMPORTED)
#set_target_properties(SailExtra-${LIB_0} PROPERTIES
#   IMPORTED_LOCATION "${LIB_PATH}/sail/bin/${LIB_0}.${SHARED_EXT}"
#   IMPORTED_IMPLIB "${EXTERN_PATH}/sail/extra/B/lib/${LIB_1}.${LIB_EXT}"
#)
#target_link_libraries(Sail INTERFACE SailExtra-${LIB_0})
#endforeach()

# Copy over the stupid number of shared libraries that Sail demands 
# foreach(LIB IN LISTS SAIL_EXTRA_LIBS)
# add_custom_command(TARGET SLZTextureProcessor POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy "${LIB_PATH}/sail/bin/${LIB}.${SHARED_EXT}" $<TARGET_FILE_DIR:SLZTextureProcessor>)
# endforeach()
