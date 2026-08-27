using System.Collections;
using System.IO;
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using UnityEngine;
using UnityEditor;
using UnityEngine.Experimental.Rendering;
using Unity.Collections;
using UnityEditor.AssetImporters;
using static SLZ.SLZTextureProcessor.NativeShared;
using static SLZ.SLZTextureProcessor.NativeBindings;

using Debug = UnityEngine.Debug;
using UnityEditor.Build;
using Unity.Mathematics;
using System.Reflection;
using UnityEditor.ProjectWindowCallback;
using UnityEditorInternal;

namespace SLZ.SLZTextureProcessor
{
    [ScriptedImporter(SlzAysxTextureImporter.currentVersion, new string[] { ".aysx" }, new string[] {}, AllowCaching = true)]
    public class SlzAysxTextureImporter : ScriptedImporter
    {
        internal const int currentVersion = 6;

        [SerializeField]
        [HideInInspector]
        private int SerializedVersion = currentVersion;

        public LazyLoadReference<Texture2D> normalMapRef;
        public LazyLoadReference<Texture2D> masMapRef;

        public TextureImporterSwizzle normalMapSwizzleR = TextureImporterSwizzle.R;
        public TextureImporterSwizzle normalMapSwizzleG = TextureImporterSwizzle.G;
        public TextureImporterSwizzle normalMapSwizzleB = TextureImporterSwizzle.B;
        public TextureImporterSwizzle normalMapSwizzleA = TextureImporterSwizzle.A;

        public TextureImporterSwizzle masMapSwizzleR = TextureImporterSwizzle.R;
        public TextureImporterSwizzle masMapSwizzleG = TextureImporterSwizzle.G;
        public TextureImporterSwizzle masMapSwizzleB = TextureImporterSwizzle.B;
        public TextureImporterSwizzle masMapSwizzleA = TextureImporterSwizzle.A;

        //public SLZTextureType type = SLZTextureType.normalMap;
        public bool generateMips = true;
        public bool streamingMipmaps = true;
        public int streamingMipmapsPriority = 0;
        public string mipmapLimitGroupName;
        public bool ignoreMipmapLimit = false;


        public FilterMode filtering = FilterMode.Bilinear;

        public float mipBias = 0.0f;
        public TextureWrapMode wrapU;
        public TextureWrapMode wrapV;
        public TextureWrapMode wrapW;
        public TextureImporterNPOTScale NPOTScale = TextureImporterNPOTScale.ToNearest;

        public TextureCompressionQuality compressionQuality = TextureCompressionQuality.Best;

        public bool hemiOctahedralEncoding = true;
        public bool geometricRoughness = true;
        public bool isReadable = false;
        public float geoRoughnessStrength = 1.0f;

        [Serializable]
        public struct TxpPlatformSettings
        {
            public string buildTarget;
            public bool overridden;
            public int maxSize;
            public TextureImporterCompression textureCompression;
            public TextureFormat textureFormat;
            public TextureCompressionQuality compressionQuality;
            public int aniso;
        }

        public List<TxpPlatformSettings> txpPlatformSettings = new List<TxpPlatformSettings>();

        public TextureFormat GetDefaultFormatForPlatform(string platformName, bool hasAlpha, bool isHDR, bool isNormal)
        {
            if (platformName == "Android" || platformName == "iOS")
            {
                if (isHDR && !isNormal)
                {
                    switch (compressionQuality)
                    {
                        case (TextureCompressionQuality.Best): return TextureFormat.ASTC_HDR_4x4;
                        case (TextureCompressionQuality.Normal): return TextureFormat.ASTC_HDR_5x5;
                        case (TextureCompressionQuality.Fast): return TextureFormat.ASTC_HDR_6x6;
                        default: return TextureFormat.ASTC_HDR_4x4;
                    }
                }
                else
                {
                    switch (compressionQuality)
                    {
                        case (TextureCompressionQuality.Best): return TextureFormat.ASTC_4x4;
                        case (TextureCompressionQuality.Normal): return TextureFormat.ASTC_5x5;
                        case (TextureCompressionQuality.Fast): return TextureFormat.ASTC_6x6;
                        default: return TextureFormat.ASTC_4x4;
                    }
                }
            }
            // PC and everything not mobile
            else
            {
                return TextureFormat.BC7;
            }
        }

        private void PopulatePlatformSettings()
        {
            if (txpPlatformSettings == null)
            {
                txpPlatformSettings = new List<TxpPlatformSettings>();
            }
            if (txpPlatformSettings.Count == 0 || txpPlatformSettings[0].buildTarget != "DefaultTexturePlatform")
            {
                txpPlatformSettings.Insert(0,
                new TxpPlatformSettings
                {
                    buildTarget = "DefaultTexturePlatform",
                    overridden = false,
                    maxSize = 2048,
                    textureCompression = TextureImporterCompression.CompressedHQ,
                    textureFormat = (TextureFormat)(-1),
                    compressionQuality = TextureCompressionQuality.Best,
                    aniso = 1
                });
            }
            BuildPlatformsMirror.buildPlatformInfo[] platforms = BuildPlatformsMirror.ValidBuildPlatforms;
            int platformsCount = platforms.Length;
            int txpPlatCount = txpPlatformSettings.Count;
            for (int pIdx = 0; pIdx < platformsCount; pIdx++)
            {
                string platName = platforms[pIdx].buildTarget.TargetName;
                bool foundPlatform = false;
                for (int tIdx = 1; tIdx < txpPlatCount; tIdx++)
                {
                    if (txpPlatformSettings[tIdx].buildTarget == platName)
                    {
                        foundPlatform = true;
                        break;
                    }
                }
                if (!foundPlatform)
                {
                    txpPlatformSettings.Add(
                    new TxpPlatformSettings
                    {
                        buildTarget = platforms[pIdx].buildTarget.TargetName,
                        overridden = false,
                        maxSize = 2048,
                        textureCompression = TextureImporterCompression.Compressed,
                        textureFormat = (TextureFormat)(-1),
                        compressionQuality = TextureCompressionQuality.Best,
                        aniso = 1
                    }
                    );
                }
            }
        }

        int GetPlatformSettingsIndex(string targetName)
        {
            int numTargets = txpPlatformSettings.Count;
            if (numTargets == 0)
            {
                PopulatePlatformSettings();
            }
            for (int tIdx = 1; tIdx < numTargets; tIdx++)
            {
                if (txpPlatformSettings[tIdx].buildTarget == targetName)
                {
                    if (txpPlatformSettings[tIdx].overridden)
                    {
                        return tIdx;
                    }
                    break;
                }
            }
            return 0;
        }

        int GetLightmapFormatForNormalMap(TextureFormat fmt)
        {
            switch (fmt)
            {
                case (TextureFormat.BC7):
                case (TextureFormat.DXT5):
                case (TextureFormat.DXT5Crunched):
                    return 3;
                case (TextureFormat.BC5):
                case (TextureFormat.BC6H):
                    return 4;
                default: return 4;
            }
        }

        bool ShouldUseDXTnmEncoding(TextureFormat fmt, SLZTextureType type, bool hasAlpha)
        {
            if (type == SLZTextureType.detailMap)
            {
                return true;
            }
            if (!hasAlpha)
            {
                return false;
            }
            switch (fmt)
            {
                case TextureFormat.BC7:
                case TextureFormat.DXT5:
                case TextureFormat.DXT5Crunched:
                    return true;
                default: return false;
            }
        }

        public override void OnImportAsset(AssetImportContext ctx)
        {
            SerializedVersion = currentVersion;
            //string assetPath = Path.GetFullPath(ctx.assetPath);
            
            if (!normalMapRef.isSet || normalMapRef.isBroken || !masMapRef.isSet || masMapRef.isBroken)
            {
                Texture2D dummy = new Texture2D(64, 64, TextureFormat.R8, false, false);
                ctx.AddObjectToAsset("texture", dummy);
                ctx.SetMainObject(dummy);
                return;
            }

            string normalMapLocalPath = AssetDatabase.GetAssetPath(normalMapRef.instanceID);
            string normalMapAssetPath = Path.GetFullPath(normalMapLocalPath);
            string masMapLocalPath = AssetDatabase.GetAssetPath(masMapRef.instanceID);
            string masMapAssetPath = Path.GetFullPath(AssetDatabase.GetAssetPath(masMapRef.instanceID));

            ctx.DependsOnSourceAsset(normalMapLocalPath);
            ctx.DependsOnSourceAsset(masMapLocalPath);

            //Debug.Log($"Import paths: \n{normalMapAssetPath}\n{masMapAssetPath}");
            SLZTexProcPluginLogger.InitializeLogger();

            BuildTarget target = ctx.selectedBuildTarget;
            string targetName = NamedBuildTarget.FromBuildTargetGroup(BuildPipeline.GetBuildTargetGroup(target)).TargetName;
            int settingIdx = GetPlatformSettingsIndex(targetName);
            TxpPlatformSettings settings = txpPlatformSettings[settingIdx];

            ImageFileHandler nrmExportInfo = TxpGetImageInfo(normalMapAssetPath, normalMapAssetPath.Length);
            //Debug.Log($"Normal ExportInfo:\n    size: {nrmExportInfo.width} x {nrmExportInfo.height}\n    Format: {nrmExportInfo.textureFormat}");

            if (nrmExportInfo.textureFormat == TxpTextureFormat.FMT_UNKNOWN ||
                nrmExportInfo.ImageIOHandler == IntPtr.Zero ||
                nrmExportInfo.width == 0 ||
                nrmExportInfo.height == 0)
            {
                Debug.LogError($"SLZ Texture Importer: Failed to import normal map texure at path: {normalMapAssetPath}");
                Texture2D dummy = new Texture2D(64, 64, TextureFormat.R8, false, false);
                ctx.AddObjectToAsset("texture", dummy);
                ctx.SetMainObject(dummy);
                TxpDisposeImageInfo(nrmExportInfo);
                return;
            }

            ImageFileHandler masExportInfo = TxpGetImageInfo(masMapAssetPath, masMapAssetPath.Length);
            //Debug.Log($"MAS ExportInfo:\n    size: {masExportInfo.width} x {masExportInfo.height}\n    Format: {masExportInfo.textureFormat}");

            if (masExportInfo.textureFormat == TxpTextureFormat.FMT_UNKNOWN ||
                masExportInfo.ImageIOHandler == IntPtr.Zero ||
                masExportInfo.width == 0 ||
                masExportInfo.height == 0)
            {
                Debug.LogError($"SLZ Texture Importer: Failed to import normal map texure at path: {masMapAssetPath}");
                Texture2D dummy = new Texture2D(64, 64, TextureFormat.R8, false, false);
                ctx.AddObjectToAsset("texture", dummy);
                ctx.SetMainObject(dummy);
                TxpDisposeImageInfo(nrmExportInfo);
                return;
            }

            TextureFormat rawNrmTexFmt = GetFmtFromExport(nrmExportInfo.textureFormat);
            GraphicsFormat rawNrmGfxFmt = GraphicsFormatUtility.GetGraphicsFormat(rawNrmTexFmt, false);

            // Uncompressed RGBA image types should always be supported (hopefully)
            if (!SystemInfo.IsFormatSupported(rawNrmGfxFmt, FormatUsage.Sample))
            {
                rawNrmGfxFmt = GraphicsFormatUtility.ConvertToAlphaFormat(rawNrmGfxFmt);
            }

            //Debug.Log($"SLZTextureImporter: Uncompressed texture format: {Enum.GetName(typeof(GraphicsFormat), rawNrmGfxFmt)}");
            bool inputIsHDR = TxpFmtIsHDR(nrmExportInfo.textureFormat);
            bool requiresAlpha = true;


            TextureFormat outputFormat;
            if (settings.textureFormat == (TextureFormat)(-1))
            {
                outputFormat = GetDefaultFormatForPlatform(targetName, requiresAlpha, inputIsHDR, true);
            }
            else
            {
                outputFormat = settings.textureFormat;
            }

            bool hasAlpha = UnityEngine.Experimental.Rendering.GraphicsFormatUtility.HasAlphaChannel(outputFormat);
            int useDXTnm = 1;

            double largestDim = Math.Max(nrmExportInfo.width, nrmExportInfo.height);
            double scaleClamp = largestDim > settings.maxSize ? settings.maxSize / largestDim : 1;

            double width = scaleClamp * (double)nrmExportInfo.width;
            double height = scaleClamp * (double)nrmExportInfo.height;
            double log2Width = Math.Log(width, 2.0);
            double log2Height = Math.Log(height, 2.0);
            switch (NPOTScale)
            {
                case TextureImporterNPOTScale.ToLarger:
                    log2Width = Math.Ceiling(log2Width);
                    log2Height = Math.Ceiling(log2Height);
                    break;
                case TextureImporterNPOTScale.ToSmaller:
                    log2Width = Math.Floor(log2Width);
                    log2Height = Math.Floor(log2Height);
                    break;
                case TextureImporterNPOTScale.ToNearest:
                default:
                    log2Width = Math.Round(log2Width);
                    log2Height = Math.Round(log2Height);
                    break;
            }

            int pow2Width = (int)Math.Pow(2, log2Width);
            int pow2Height = (int)Math.Pow(2, log2Height);

            int mipCount = generateMips ? (int)Math.Max(log2Width, log2Height) + 1 : 1;

            TextureCreationFlags texFlags = TextureCreationFlags.DontInitializePixels | TextureCreationFlags.DontUploadUponCreate | (TextureCreationFlags)(1 << 9);
            if (((int)texFlags & (1 << 9)) == 0) Debug.LogError("SLZ Texture Importer: Failed to set DontCheckGraphicsCaps on texture creation flags");
            if (generateMips) texFlags |= TextureCreationFlags.MipChain;
            if (ignoreMipmapLimit) texFlags |= TextureCreationFlags.IgnoreMipmapLimit;

            Texture2D output = new Texture2D(pow2Width, pow2Height, rawNrmGfxFmt, texFlags);

            output.filterMode = filtering;
            output.mipMapBias = mipBias;

            output.wrapModeU = wrapU;
            output.wrapModeV = wrapV;
            output.wrapModeW = wrapW;
            output.anisoLevel = settings.aniso;

            //int mipCount = output.mipmapCount;
            NativeArray<IntPtr> mipChain = new NativeArray<IntPtr>(mipCount, Allocator.Temp);
            NativeArray<Vector2Int> mipResolutions = new NativeArray<Vector2Int>(mipCount, Allocator.Temp);

            for (int mIdx = 0; mIdx < mipCount; mIdx++)
            {
                mipChain[mIdx] = TxpNativeArrayIntPtr.GetIntPtr(output.GetPixelData<byte>(mIdx));
                mipResolutions[mIdx] = new Vector2Int(
                    Mathf.Max(output.width >> mIdx, 1),
                    Mathf.Max(output.height >> mIdx, 1)
                    );
            }

            // 3 channel formats not supported on textures, so the GraphicsFormat we got might have added an alpha channel
            // Thus re-translate the GraphicsFormat back to the native format to get the format with an alpha channel

            TxpTextureFormat expFmt = GetFmtForExport(rawNrmGfxFmt);

            TxpTex2D textureDesc = new TxpTex2D()
            {
                format = expFmt,
                mipCount = mipCount,
                resolution = TxpNativeArrayIntPtr.GetIntPtr(mipResolutions),
                mips = TxpNativeArrayIntPtr.GetIntPtr(mipChain),
                pad0 = 0
            };

            int4 normalMapSwizzle = new int4(
                (int) normalMapSwizzleR,
                (int) normalMapSwizzleG,
                (int) normalMapSwizzleB,
                (int) normalMapSwizzleA
                );

            int4 masMapSwizzle = new int4(
                (int)masMapSwizzleR,
                (int)masMapSwizzleG,
                (int)masMapSwizzleB,
                (int)masMapSwizzleA
                );


            TXPErrorCode errorCode = (TXPErrorCode)TxpReadAndProcessAoSmNormalMap(
                nrmExportInfo,
                masExportInfo,
                textureDesc,
                normalMapSwizzle,
                masMapSwizzle,
                0,
                1,
                2,
                hemiOctahedralEncoding ? 1 : 0,
                geometricRoughness ? geoRoughnessStrength : 0
                );

            // Generate Thumbnail
            Texture2D thumbnail = null;
            if (errorCode == TXPErrorCode.TXP_RETURN_SUCCESS)
            {
                thumbnail = new Texture2D(16, 16,
                    GraphicsFormat.R8G8B8A8_UNorm,
                    TextureCreationFlags.None);
                NativeArray<byte> thumbData = thumbnail.GetPixelData<byte>(0);
                IntPtr thumbPtr = TxpNativeArrayIntPtr.GetIntPtr(thumbData);
                int thumbmip = 0;
                int minDim = Mathf.Min(output.width, output.height);
                while (minDim > 16 && thumbmip < (mipCount - 1))
                {
                    minDim /= 2;
                    thumbmip += 1;
                }
                int2 mipDim = new int2() { x = mipResolutions[thumbmip].x, y = mipResolutions[thumbmip].y };
                IntPtr mipPtr = mipChain[thumbmip];

                int2 thumbDim = new int2()
                {
                    x = output.width > output.height ? 16 : Mathf.Max(1, (int)(16.0 * (double)output.width / (double)output.height)),
                    y = output.height > output.width ? 16 : Mathf.Max(1, (int)(16.0 * (double)output.height / (double)output.width))
                };
                int isNormal = 0;
                int4 swizzle = new int4(0, 1, 2, 3);
                TxpGenerateThumbnail(expFmt, mipPtr, mipDim, thumbPtr, thumbDim, swizzle, isNormal, hemiOctahedralEncoding ? 1 : 0);
            }


            mipResolutions.Dispose();
            mipChain.Dispose();
            TxpDisposeImageInfo(nrmExportInfo);
            TxpDisposeImageInfo(masExportInfo);

            if (errorCode != TXPErrorCode.TXP_RETURN_SUCCESS)
            {
                Debug.LogError($"SLZ Texture Importer: Failed to import texture with error code {Enum.GetName(typeof(TXPErrorCode), errorCode)} at path: {assetPath} ");
                Texture2D dummy = new Texture2D(64, 64, TextureFormat.R8, false, false);
                ctx.AddObjectToAsset("texture", dummy);
                ctx.SetMainObject(dummy);
                DestroyImmediate(output);
                return;
            }

            EditorUtility.CompressTexture(output, outputFormat, TextureCompressionQuality.Best);

            // Use hidden utility class to set the texture streaming parameters.
            // NOTE: the texture MUST be created with TextureCreationFlags.DontUploadUponCreate
            // otherwise unity will hard crash when trying to unload the texture from memory!
            // It seems to try to clean up streaming-related data that wasn't initialized.
            // The same will happen if you set the values through the SerializedProperty system
            TextureUtilMirror.SetTexture2DStreamingMipmaps(output, streamingMipmaps);
            TextureUtilMirror.SetTexture2DStreamingMipmapsPriority(output, streamingMipmapsPriority);

            SerializedObject serial = new SerializedObject(output);
            SerializedProperty lmFlags = serial.FindProperty("m_LightmapFormat");
            lmFlags.intValue = 0;

            //SerializedProperty readableProp = serial.FindProperty("m_IsReadable");
            //readableProp.boolValue = isReadable;

            serial.ApplyModifiedProperties();
            serial.Dispose();

            if (!isReadable)
            {
                output.Apply(false, true);
            }

            ctx.AddObjectToAsset("texture", output, thumbnail);
            ctx.SetMainObject(output);
        }

        internal static GraphicsFormat GetGfxFmtFromExport(TxpTextureFormat expFmt)
        {
            switch (expFmt)
            {
                case (TxpTextureFormat.FMT_R8):        return GraphicsFormat.R8_UNorm;
                case (TxpTextureFormat.FMT_RG8):       return GraphicsFormat.R8G8_UNorm;
                case (TxpTextureFormat.FMT_RGB8):      return GraphicsFormat.R8G8B8A8_UNorm;
                case (TxpTextureFormat.FMT_RGBA8):     return GraphicsFormat.R8G8B8A8_UNorm;
                case (TxpTextureFormat.FMT_R16):       return GraphicsFormat.R16_UNorm;
                case (TxpTextureFormat.FMT_RG16):      return GraphicsFormat.R16G16_UNorm;
                case (TxpTextureFormat.FMT_RGB16):     return GraphicsFormat.R16G16B16A16_UNorm;
                case (TxpTextureFormat.FMT_RGBA16):    return GraphicsFormat.R16G16B16A16_UNorm;
                case (TxpTextureFormat.FMT_RHalf):     return GraphicsFormat.R16_SFloat;
                case (TxpTextureFormat.FMT_RGHalf):    return GraphicsFormat.R16G16_SFloat;
                case (TxpTextureFormat.FMT_RGBHalf):   return GraphicsFormat.R16G16B16A16_SFloat;
                case (TxpTextureFormat.FMT_RGBAHalf):  return GraphicsFormat.R16G16B16A16_SFloat;
                case (TxpTextureFormat.FMT_RFloat):    return GraphicsFormat.R32_SFloat;
                case (TxpTextureFormat.FMT_RGFloat):   return GraphicsFormat.R32G32_SFloat;
                case (TxpTextureFormat.FMT_RGBFloat):  return GraphicsFormat.R32G32B32A32_SFloat;
                case (TxpTextureFormat.FMT_RGBAFloat): return GraphicsFormat.R32G32B32A32_SFloat;
                default: return GraphicsFormat.None;
            }
        }

        internal static TextureFormat GetFmtFromExport(TxpTextureFormat expFmt)
        {
            switch (expFmt)
            {
                case (TxpTextureFormat.FMT_R8): return TextureFormat.R8;
                case (TxpTextureFormat.FMT_RG8): return TextureFormat.RG16;
                case (TxpTextureFormat.FMT_RGB8): return TextureFormat.RGB24;
                case (TxpTextureFormat.FMT_RGBA8): return TextureFormat.RGBA32;
                case (TxpTextureFormat.FMT_R16): return TextureFormat.R16;
                case (TxpTextureFormat.FMT_RG16): return TextureFormat.RG32;
                case (TxpTextureFormat.FMT_RGB16): return TextureFormat.RGB48;
                case (TxpTextureFormat.FMT_RGBA16): return TextureFormat.RGBA64;
                case (TxpTextureFormat.FMT_RHalf): return TextureFormat.RHalf;
                case (TxpTextureFormat.FMT_RGHalf): return TextureFormat.RGHalf;
                case (TxpTextureFormat.FMT_RGBHalf): return TextureFormat.RGBAHalf;
                case (TxpTextureFormat.FMT_RGBAHalf): return TextureFormat.RGBAHalf;
                case (TxpTextureFormat.FMT_RFloat): return TextureFormat.RFloat;
                case (TxpTextureFormat.FMT_RGFloat): return TextureFormat.RGFloat;
                case (TxpTextureFormat.FMT_RGBFloat): return TextureFormat.RGBAFloat;
                case (TxpTextureFormat.FMT_RGBAFloat): return TextureFormat.RGBAFloat;
                default: return (TextureFormat)0;
            }
        }

        internal static TxpTextureFormat GetFmtForExport(GraphicsFormat unityFmt)
        {
            switch (unityFmt)
            {
                case (GraphicsFormat.R8_SRGB): return TxpTextureFormat.FMT_R8;
                case (GraphicsFormat.R8G8_SRGB): return TxpTextureFormat.FMT_RG8;
                case (GraphicsFormat.R8G8B8_SRGB): return TxpTextureFormat.FMT_RGB8;
                case (GraphicsFormat.R8G8B8A8_SRGB): return TxpTextureFormat.FMT_RGBA8;
                case (GraphicsFormat.R8_UNorm): return TxpTextureFormat.FMT_R8;
                case (GraphicsFormat.R8G8_UNorm): return TxpTextureFormat.FMT_RG8;
                case (GraphicsFormat.R8G8B8_UNorm): return TxpTextureFormat.FMT_RGB8;
                case (GraphicsFormat.R8G8B8A8_UNorm): return TxpTextureFormat.FMT_RGBA8;
                case (GraphicsFormat.R16_UNorm): return TxpTextureFormat.FMT_R16;
                case (GraphicsFormat.R16G16_UNorm): return TxpTextureFormat.FMT_RG16;
                case (GraphicsFormat.R16G16B16_UNorm): return TxpTextureFormat.FMT_RGB16;
                case (GraphicsFormat.R16G16B16A16_UNorm): return TxpTextureFormat.FMT_RGBA16;
                case (GraphicsFormat.R16_SFloat): return TxpTextureFormat.FMT_RHalf;
                case (GraphicsFormat.R16G16_SFloat): return TxpTextureFormat.FMT_RGHalf;
                case (GraphicsFormat.R16G16B16_SFloat): return TxpTextureFormat.FMT_RGBHalf;
                case (GraphicsFormat.R16G16B16A16_SFloat): return TxpTextureFormat.FMT_RGBAHalf;
                case (GraphicsFormat.R32_SFloat): return TxpTextureFormat.FMT_RFloat;
                case (GraphicsFormat.R32G32_SFloat): return TxpTextureFormat.FMT_RGFloat;
                case (GraphicsFormat.R32G32B32_SFloat): return TxpTextureFormat.FMT_RGBFloat;
                case (GraphicsFormat.R32G32B32A32_SFloat): return TxpTextureFormat.FMT_RGBAFloat;
                default: return TxpTextureFormat.FMT_UNKNOWN;
            }
        }

        internal static bool TxpFmtHasAlpha(TxpTextureFormat expFmt)
        {
            switch (expFmt)
            {
                case (TxpTextureFormat.FMT_R8): return false;
                case (TxpTextureFormat.FMT_RG8): return false;
                case (TxpTextureFormat.FMT_RGB8): return false;
                case (TxpTextureFormat.FMT_RGBA8): return true;
                case (TxpTextureFormat.FMT_R16): return false;
                case (TxpTextureFormat.FMT_RG16): return false;
                case (TxpTextureFormat.FMT_RGB16): return false;
                case (TxpTextureFormat.FMT_RGBA16): return true;
                case (TxpTextureFormat.FMT_RHalf): return false;
                case (TxpTextureFormat.FMT_RGHalf): return false;
                case (TxpTextureFormat.FMT_RGBHalf): return false;
                case (TxpTextureFormat.FMT_RGBAHalf): return true;
                case (TxpTextureFormat.FMT_RFloat): return false;
                case (TxpTextureFormat.FMT_RGFloat): return false;
                case (TxpTextureFormat.FMT_RGBFloat): return false;
                case (TxpTextureFormat.FMT_RGBAFloat): return true;
                default: return false;
            }
        }

        internal static bool TxpFmtIsHDR(TxpTextureFormat expFmt)
        {
            return expFmt > TxpTextureFormat.FMT_RGBA16 ? true : false;
        }

        internal static TxpTextureChannelFmt GetChannelFmtFromTxp(TxpTextureFormat expFmt)
        {
            switch (expFmt)
            {
                case (TxpTextureFormat.FMT_R8):
                case (TxpTextureFormat.FMT_RG8):
                case (TxpTextureFormat.FMT_RGB8):
                case (TxpTextureFormat.FMT_RGBA8): return TxpTextureChannelFmt.CHANNEL_FMT_FIXED8;
                case (TxpTextureFormat.FMT_R16):
                case (TxpTextureFormat.FMT_RG16):
                case (TxpTextureFormat.FMT_RGB16):
                case (TxpTextureFormat.FMT_RGBA16): return TxpTextureChannelFmt.CHANNEL_FMT_FIXED16;
                case (TxpTextureFormat.FMT_RHalf):
                case (TxpTextureFormat.FMT_RGHalf):
                case (TxpTextureFormat.FMT_RGBHalf):
                case (TxpTextureFormat.FMT_RGBAHalf): return TxpTextureChannelFmt.CHANNEL_FMT_FLOAT16;
                case (TxpTextureFormat.FMT_RFloat):
                case (TxpTextureFormat.FMT_RGFloat):
                case (TxpTextureFormat.FMT_RGBFloat):
                case (TxpTextureFormat.FMT_RGBAFloat): return TxpTextureChannelFmt.CHANNEL_FMT_FLOAT32;
                default: return TxpTextureChannelFmt.CHANNEL_FMT_UNKNOWN;
            }
        }

        internal static TxpTextureChannelFmt GetChannelFmtFromGfx(GraphicsFormat unityFmt)
        {
            switch (unityFmt)
            {
                case (GraphicsFormat.R8_SRGB):
                case (GraphicsFormat.R8G8_SRGB):
                case (GraphicsFormat.R8G8B8_SRGB):
                case (GraphicsFormat.R8G8B8A8_SRGB):
                case (GraphicsFormat.R8_UNorm):
                case (GraphicsFormat.R8G8_UNorm):
                case (GraphicsFormat.R8G8B8_UNorm):
                case (GraphicsFormat.R8G8B8A8_UNorm): return TxpTextureChannelFmt.CHANNEL_FMT_FIXED8;
                case (GraphicsFormat.R16_UNorm):
                case (GraphicsFormat.R16G16_UNorm):
                case (GraphicsFormat.R16G16B16_UNorm):
                case (GraphicsFormat.R16G16B16A16_UNorm): return TxpTextureChannelFmt.CHANNEL_FMT_FIXED16;
                case (GraphicsFormat.R16_SFloat):
                case (GraphicsFormat.R16G16_SFloat):
                case (GraphicsFormat.R16G16B16_SFloat):
                case (GraphicsFormat.R16G16B16A16_SFloat): return TxpTextureChannelFmt.CHANNEL_FMT_FLOAT16;
                case (GraphicsFormat.R32_SFloat):
                case (GraphicsFormat.R32G32_SFloat):
                case (GraphicsFormat.R32G32B32_SFloat):
                case (GraphicsFormat.R32G32B32A32_SFloat): return TxpTextureChannelFmt.CHANNEL_FMT_FLOAT32;
                default: return TxpTextureChannelFmt.CHANNEL_FMT_UNKNOWN;
            }
        }
        internal static GraphicsFormat GetGfxFormatFromChannels(TxpTextureChannelFmt channelFmt, int channelCount, bool sRGB)
        {
            switch (channelCount)
            {
                case 4:
                case 3:
                    switch (channelFmt)
                    {
                        case (TxpTextureChannelFmt.CHANNEL_FMT_FIXED8): return sRGB ? GraphicsFormat.R8G8B8A8_UNorm : GraphicsFormat.R8G8B8A8_SRGB;
                        case (TxpTextureChannelFmt.CHANNEL_FMT_FIXED16): return GraphicsFormat.R16G16B16A16_UNorm;
                        case (TxpTextureChannelFmt.CHANNEL_FMT_FLOAT16): return GraphicsFormat.R16G16B16A16_SFloat;
                        case (TxpTextureChannelFmt.CHANNEL_FMT_FLOAT32): return GraphicsFormat.R32G32B32A32_SFloat;
                    }
                    break;
                //case 3:
                //    switch (channelFmt)
                //    {
                //        case (TxpTextureChannelFmt.CHANNEL_FMT_FIXED8): return sRGB ? GraphicsFormat.R8G8B8_UNorm : GraphicsFormat.R8G8B8_SRGB;
                //        case (TxpTextureChannelFmt.CHANNEL_FMT_FIXED16): return GraphicsFormat.R16G16B16A16_UNorm;
                //        case (TxpTextureChannelFmt.CHANNEL_FMT_FLOAT16): return GraphicsFormat.R16G16B16A16_SFloat;
                //        case (TxpTextureChannelFmt.CHANNEL_FMT_FLOAT32): return GraphicsFormat.R32G32B32A32_SFloat;
                //    }
                //    break;
                case 2:
                    switch (channelFmt)
                    {
                        case (TxpTextureChannelFmt.CHANNEL_FMT_FIXED8): return sRGB ? GraphicsFormat.R8G8_UNorm : GraphicsFormat.R8G8_SRGB;
                        case (TxpTextureChannelFmt.CHANNEL_FMT_FIXED16): return GraphicsFormat.R16G16_UNorm;
                        case (TxpTextureChannelFmt.CHANNEL_FMT_FLOAT16): return GraphicsFormat.R16G16_SFloat;
                        case (TxpTextureChannelFmt.CHANNEL_FMT_FLOAT32): return GraphicsFormat.R32G32_SFloat;
                    }
                    break;
                case 1:
                    switch (channelFmt)
                    {
                        case (TxpTextureChannelFmt.CHANNEL_FMT_FIXED8): return sRGB ? GraphicsFormat.R8_UNorm : GraphicsFormat.R8_SRGB;
                        case (TxpTextureChannelFmt.CHANNEL_FMT_FIXED16): return GraphicsFormat.R16_UNorm;
                        case (TxpTextureChannelFmt.CHANNEL_FMT_FLOAT16): return GraphicsFormat.R16_SFloat;
                        case (TxpTextureChannelFmt.CHANNEL_FMT_FLOAT32): return GraphicsFormat.R32_SFloat;
                    }
                    break;

            }
            return (GraphicsFormat)0;
        }
    }
}
