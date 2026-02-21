cmake -B build-linux -G Ninja -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=RelWithDebInfo -DUNITY_PLUGIN_HEADERS=/mnt/c/Unity/6000.3.7f1/Editor/Data/PluginAPI -DPRINT_HELP=OFF -DVulkan_LIBRARY=/mnt/c/VulkanSDK/1.4.309.0/Lib -DVulkan_INCLUDE_DIR=/mnt/c/VulkanSDK/1.4.309.0/Include
cd build-linux
ninja