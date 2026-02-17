using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using Unity.Mathematics;
using static SLZ.SLZTextureProcessor.NativeShared;

namespace SLZ.SLZTextureProcessor
{
    internal static class NativeBindings
    {
        const string PLUGIN_NAME = "SLZTextureProcessor";

        [DllImport(PLUGIN_NAME, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void TxpSetLogger(IntPtr logMethodPtr);

        [DllImport(PLUGIN_NAME, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Auto)]
        internal static extern ExportImageInfo TxpGetImageInfo(
           string path,
           int pathLength
           );

        [DllImport(PLUGIN_NAME, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void TxpDisposeImageInfo(ExportImageInfo exportInfo);

        [DllImport(PLUGIN_NAME, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int TxpReadAndProcessNormalMap(ExportImageInfo exportInfo, TxpTex2D texture, int inDXTnm, int outDXTnm, int isDetail, int isHemiOct, float geoRoughStr);

        [DllImport(PLUGIN_NAME, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int TxpProcessNormalMap(IntPtr texInBuffer, int texInWidth, int texInHeight, TxpTextureFormat texInFmt, TxpTex2D texOut, int inDXTnm, int outDXTnm, int isDetail, int isHemiOct, float geoRoughStr);

        [DllImport(PLUGIN_NAME, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int TxpGenerateThumbnail(TxpTextureFormat formatIn, IntPtr imageIn, int2 resIn, IntPtr imageOut, int2 resOut, int4 swizzle, int isNormal, int hemiOct);

    }
}
