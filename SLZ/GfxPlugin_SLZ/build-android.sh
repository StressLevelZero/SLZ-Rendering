#!/bin/bash

Help()
{
   echo "Build graphics plugin for android"
   echo
   echo "Syntax: mode [-c|h|u|m]"
   echo "options:"
   echo "c     Only setup the visual studio project with cmake, do not build"
   echo "h     Print this Help."
   echo "u     Full path of the Unity PluginAPI folder"
   echo "m     Build mode (Release, RelWithDebInfo, Debug)"
   echo "      (usually in .../Unity/VERSION_NUMBER/Editor/Data/PluginAPI)"
   echo
}

mode=""
do_build=true
UNITY_PLUGIN_HEADERS=""

while getopts "hcu:m:" option; do
   case $option in
      h) # display Help
         Help
         exit 1
         ;;
         
      c) # don't build
        echo "Should skip build"
        do_build=false 
        ;;
        
      u) #unity plugin API path
        UNITY_PLUGIN_HEADERS=$OPTARG 
        ;;
        
      m) # Build mode
        mode=$OPTARG
   esac
done


if [ -z $mode ]; then
    mode=RelWithDebInfo
fi

njobs=`nproc`

if [ -z $UNITY_PLUGIN_HEADERS ]; then
    UNITY_PLUGIN_HEADERS="C:/Unity/6000.3.7f1/Editor/Data/PluginAPI"
fi

# Android
if [ -z $ANDROID_SDK ]; then
    ANDROID_SDK="C:/utilities/AndroidSDK"
fi

ANDROID_NDK_VERSION="27.0.11718014" 

mkdir -p build-android-arm64-v8a
cd build-android-arm64-v8a

ANDROID_ABI=arm64-v8a
"$ANDROID_SDK"/cmake/*/bin/cmake \
    .. \
    -G Ninja \
    -DANDROID_STL=c++_static \
    -DANDROID_TOOLCHAIN=clang \
    -DCMAKE_TOOLCHAIN_FILE="$ANDROID_SDK/ndk/${ANDROID_NDK_VERSION}/build/cmake/android.toolchain.cmake" \
    -DANDROID_ABI=$ANDROID_ABI \
    -DANDROID_CPP_FEATURES=exceptions \
    -DANDROID_ARM_MODE=arm \
    -DANDROID_PLATFORM=31 \
    -DCMAKE_BUILD_TYPE=$mode \
    -DPRINT_HELP=OFF \
    -DUNITY_PLUGIN_HEADERS="$UNITY_PLUGIN_HEADERS" \
    -DBUILD_TARGET:STRING=ANDROID \
    -DANDROID_SUPPORT_FLEXIBLE_PAGE_SIZES=ON
    
if test "$do_build" = true; then
    ninja
fi

cd ..