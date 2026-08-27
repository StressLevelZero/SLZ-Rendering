using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;
using UnityEngine;
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
        internal static extern ImageFileHandler TxpGetImageInfo(
           string path,
           int pathLength
           );

        [DllImport(PLUGIN_NAME, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void TxpDisposeImageInfo(ImageFileHandler exportInfo);

        [DllImport(PLUGIN_NAME, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int TxpReadAndProcessNormalMap(ImageFileHandler exportInfo, TxpTex2D texture, int inDXTnm, int outDXTnm, int isDetail, int isHemiOct, float geoRoughStr);

        [DllImport(PLUGIN_NAME, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int TxpProcessNormalMap(IntPtr texInBuffer, int texInWidth, int texInHeight, TxpTextureFormat texInFmt, TxpTex2D texOut, int inDXTnm, int outDXTnm, int isDetail, int isHemiOct, float geoRoughStr);

        [DllImport(PLUGIN_NAME, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int TxpGenerateThumbnail(TxpTextureFormat formatIn, IntPtr imageIn, int2 resIn, IntPtr imageOut, int2 resOut, int4 swizzle, int isNormal, int hemiOct);

        [DllImport(PLUGIN_NAME, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int TxpReadAndProcessAoSmNormalMap(
            ImageFileHandler nrmExportInfo,
            ImageFileHandler masExportInfo,
            TxpTex2D texOut, 
            int4 swizzleNormal,
            int4 swizzleMAS,
            int inDXTnm, int outDXTnm, int isDetail, int isHemiOct, float geoRoughStr);

        [DllImport(PLUGIN_NAME, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int TxpReadFile(ImageFileHandler exportInfo);

        [DllImport(PLUGIN_NAME, CallingConvention = CallingConvention.Cdecl)]
        internal static extern TxpTexSubresource GetSubresourceFromIO(ImageFileHandler info);

        [DllImport(PLUGIN_NAME, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int ImageRequiresYFlip(ImageFileHandler info);

        [DllImport(PLUGIN_NAME, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int TxpYFlipSubresource(TxpTexSubresource texture);

        [DllImport(PLUGIN_NAME, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int ConvertSwizzleSubresource(TxpTexSubresource textureIn, TxpTexSubresource textureOut, float4x4 swizzleMatrix, float4 swizzleAdd);

        [DllImport(PLUGIN_NAME, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Auto)]
        internal static extern int TxpSaveImageToPNG(string savePath, TxpTexSubresource image, int compressionLevel);
    }
}
