#!/bin/bash

Help()
{
   echo "Build graphics plugin for linux"
   echo
   echo "Syntax: mode [-c|h|u|v|m]"
   echo "options:"
   echo "c     Only setup the project with cmake, do not build"
   echo "h     Print this Help."
   echo "u     Full path of the Unity PluginAPI folder"
   echo "      (usually in .../Unity/VERSION_NUMBER/Editor/Data/PluginAPI)"
   echo "v     Full path of the Vulkan SDK folder"
   echo "m     Build mode (Release, RelWithDebInfo, Debug)"
   echo
}

mode=""
do_build=true
UNITY_PLUGIN_HEADERS=""
VULKAN_SDK_PATH=""

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
        
      v) #Vulkan SDK path
        VULKAN_SDK_PATH=$OPTARG 
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
    UNITY_PLUGIN_HEADERS="/mnt/c/Unity/6000.3.7f1/Editor/Data/PluginAPI"
fi

mkdir -p build-linux-x64
cd build-linux-x64

cmake \
 .. \
 -B $mode \
 -G Ninja \
 -DCMAKE_C_COMPILER=clang \
 -DCMAKE_CXX_COMPILER=clang++ \
 -DCMAKE_BUILD_TYPE=$mode \
 -DUNITY_PLUGIN_HEADERS=$UNITY_PLUGIN_HEADERS \
 -DPRINT_HELP=OFF \
 -DVulkan_LIBRARY=$VULKAN_SDK_PATH/Lib \
 -DVulkan_INCLUDE_DIR=$VULKAN_SDK_PATH/Include \

cd $mode

if test "$do_build" = true; then
    ninja
fi

cd ..