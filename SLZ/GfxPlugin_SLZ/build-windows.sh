#!/bin/bash

Help()
{
   echo "Build graphics plugin for linux"
   echo
   echo "Syntax: mode [-h|u|m]"
   echo "options:"
   echo "h     Print this Help."
   echo "u     Full path of the Unity PluginAPI folder"
   echo "      (usually in .../Unity/VERSION_NUMBER/Editor/Data/PluginAPI)"
   echo "m     Build mode (Release, RelWithDebInfo, Debug)"
   echo
}

mode=""
UNITY_PLUGIN_HEADERS=""

while getopts "hcu:m:" option; do
   case $option in
      h) # display Help
         Help
         exit 1
         ;;
        
      u) #unity plugin API path
        UNITY_PLUGIN_HEADERS=$OPTARG 
        ;;
        
      m) # Build mode
        mode=$OPTARG
   esac
done

if [ -z $UNITY_PLUGIN_HEADERS ]; then
    UNITY_PLUGIN_HEADERS="C:\Unity\6000.3.7f1\Editor\Data\PluginAPI"
fi

if [ -z $mode ]; then
    mode=RelWithDebInfo
fi

njobs=`nproc`

if [ -z $UNITY_PLUGIN_HEADERS ]; then
    UNITY_PLUGIN_HEADERS="/mnt/c/Unity/6000.3.7f1/Editor/Data/PluginAPI"
fi

mkdir -p build-windows-x64
cd build-windows-x64

cmake \
 .. \
 -B build-windows-x64 \
 -G "Visual Studio 17 2022" \
 -T ClangCl \
 -DCMAKE_BUILD_TYPE=$mode \
 -DUNITY_PLUGIN_HEADERS=$UNITY_PLUGIN_HEADERS \
 -DPRINT_HELP=OFF \
 
cd ..