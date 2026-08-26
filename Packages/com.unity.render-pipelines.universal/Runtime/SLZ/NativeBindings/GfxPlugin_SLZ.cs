using System.Runtime.InteropServices;
using System;
using UnityEngine;

namespace SLZ
{
    public class GfxPlugin_SLZ
    {
        const string pluginName = "GfxPlugin_SLZ";
        #if !UNITY_ANDROID
        [DllImport(pluginName, EntryPoint="SupportsLayeredShadingRate")]
        public static extern UInt32 SupportsLayeredShadingRate_Extern();
        #endif
        private static bool supportsLayeredShadingRate_cached = false;
        private static bool SupportsLayeredShadingRate_value = false;

        public static bool SupportsLayeredShadingRate()
        {
            #if !UNITY_ANDROID
            if (!supportsLayeredShadingRate_cached)
            {
                SupportsLayeredShadingRate_value = SupportsLayeredShadingRate_Extern() != 0;
                supportsLayeredShadingRate_cached = true;
            }
            return SupportsLayeredShadingRate_value;
            #else
            return true;
            #endif
        }

    }
}
