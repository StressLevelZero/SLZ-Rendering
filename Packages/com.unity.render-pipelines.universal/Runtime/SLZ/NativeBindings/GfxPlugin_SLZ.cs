using System.Runtime.InteropServices;
using System;
using UnityEngine;

namespace SLZ
{
    public class GfxPlugin_SLZ
    {
        const string pluginName = "GfxPlugin_SLZ";
        [DllImport(pluginName)]
        public static extern UInt32 SupportsLayeredShadingRate();
    }
}
