using System;
using System.IO;
using UnityEditor;
using UnityEngine;
using UnityEngine.Experimental.Rendering;
using Unity.Collections;

using static SLZ.SLZTextureProcessor.NativeShared;
using static SLZ.SLZTextureProcessor.NativeBindings;
using static SLZ.SLZTextureProcessor.ExtraTextureSettings;
using Debug = UnityEngine.Debug;
using static UnityEngine.GraphicsBuffer;
using UnityEditor.Build;

namespace SLZ.SLZTextureProcessor
{
    internal class TexturePostProcessor : AssetPostprocessor
    {
        public override int GetPostprocessOrder()
        {
            return 0; //Execute this first since we ignore anything done by unity and directly import the texture from source
        }

        public override uint GetVersion()
        {
            return 5u;
        }

        void OnPreprocessTexture()
        {
            TextureImporter importer = this.assetImporter as TextureImporter;
            if (this.assetPath.ToLower().StartsWith("packages")) // Don't try to modify packages!
            {
                return;
            }
            ExtraTextureSettings texSettings;
            ExtraTextureSettings.ReturnCase c = ExtraTextureSettings.TryReadFromUserData(importer.userData, out texSettings);

            if (c == ExtraTextureSettings.ReturnCase.Fail)
            {
                texSettings = new ExtraTextureSettings();

                string filename = Path.GetFileNameWithoutExtension(this.assetPath).ToLower();
                if (filename.EndsWith("_detailmap"))
                {
                    texSettings.detailMap = true;
                    texSettings.hemiOctNormals = false;
                }
                string newUserData = texSettings.Serialize();
                importer.userData = newUserData;
            }
            else if (c == ExtraTextureSettings.ReturnCase.OldXml)
            {
                importer.userData = texSettings.Serialize();
            }

            //Check for good compression settings, changing them to better ones if necessary
            if (importer.textureType == TextureImporterType.NormalMap)
            {
                TextureImporterPlatformSettings pcSettings = importer.GetPlatformTextureSettings("Standalone");
                TextureImporterPlatformSettings androidSettings = importer.GetPlatformTextureSettings("Android");
                TextureImporterPlatformSettings defaultSettings = importer.GetDefaultPlatformTextureSettings();

                if (!pcSettings.overridden)
                {
                    pcSettings.overridden = true;
                    pcSettings.maxTextureSize = defaultSettings.maxTextureSize;
                    pcSettings.resizeAlgorithm = defaultSettings.resizeAlgorithm;
                    pcSettings.crunchedCompression = false;
                    pcSettings.format = TextureImporterFormat.BC7;
                    importer.SetPlatformTextureSettings(pcSettings);
                }
                if (texSettings.hemiOctNormals && texSettings.geoRoughness != GeoRoughness.GenRoughness && pcSettings.format == TextureImporterFormat.BC7)
                {
                    pcSettings.format = TextureImporterFormat.BC5;
                    importer.SetPlatformTextureSettings(pcSettings);
                }

                if (androidSettings.overridden) //We don't want ETC compressed textures! I noticed a lot of normal maps were set to ETC for some reason, so force them back to ASTC 
                {
                    if (androidSettings.format == TextureImporterFormat.ETC2_RGB4 ||
                        androidSettings.format == TextureImporterFormat.ETC2_RGB4_PUNCHTHROUGH_ALPHA ||
                        androidSettings.format == TextureImporterFormat.ETC2_RGBA8 ||
                        androidSettings.format == TextureImporterFormat.ETC2_RGBA8Crunched ||
                        androidSettings.format == TextureImporterFormat.ETC_RGB4 ||
                        androidSettings.format == TextureImporterFormat.ETC_RGB4Crunched)
                    {
                        androidSettings.format = TextureImporterFormat.ASTC_5x5;
                        importer.SetPlatformTextureSettings(androidSettings);
                    }
                }
            }
        }

        void OnPostprocessTexture(Texture2D output)
        {
            TextureImporter inputImport = this.assetImporter as TextureImporter;
            if (inputImport == null) return; // Not a real texture, happens with ies light cookies
            ExtraTextureSettings texSettings;
            ExtraTextureSettings.ReturnCase c = TryReadFromUserData(inputImport.userData, out texSettings);
            bool isDetailMap = texSettings.detailMap;
            bool isNormalMap = inputImport.textureType == TextureImporterType.NormalMap;
            bool isDXTnm = isDetailMap;
            if (!isDetailMap) isDXTnm = IsDXTnm();
            bool isHemiOct = texSettings.hemiOctNormals;
            bool geoRoughness = texSettings.geoRoughness == GeoRoughness.GenRoughness;
            float geoRoughStr = geoRoughness ? texSettings.geoRoughnessPow : 0.0f; 

            if (!isDetailMap && !isNormalMap)
            {
                return;
            }

            if (isDetailMap && texSettings.hemiOctNormals == false && (texSettings.geoRoughness == GeoRoughness.NoRoughness || texSettings.geoRoughness == GeoRoughness.PreserveZ))
            {
                return;
            }

            SLZTexProcPluginLogger.InitializeLogger();
            int mipCount = output.mipmapCount;

            //int mipCount = output.mipmapCount;
            NativeArray<IntPtr> mipChain = new NativeArray<IntPtr>(mipCount, Allocator.Temp);
            NativeArray<Vector2Int> mipResolutions = new NativeArray<Vector2Int>(mipCount, Allocator.Temp);
            NativeArray<byte> mip0 = output.GetPixelData<byte>(0);
            for (int mIdx = 0; mIdx < mipCount; mIdx++) 
            {
                mipChain[mIdx] = TxpNativeArrayIntPtr.GetIntPtr(mIdx == 0 ? mip0 : output.GetPixelData<byte>(mIdx));
                mipResolutions[mIdx] = new Vector2Int(
                    Mathf.Max(output.width >> mIdx, 1),
                    Mathf.Max(output.height >> mIdx, 1)
                    );
            }


            TxpTextureFormat expFmt = SLZTextureImporter.GetFmtForExport(output.graphicsFormat);

            TxpTex2D textureDesc = new TxpTex2D()
            {
                format = expFmt,
                mipCount = mipCount,
                resolution = TxpNativeArrayIntPtr.GetIntPtr(mipResolutions),
                mips = TxpNativeArrayIntPtr.GetIntPtr(mipChain),
                pad0 = 0
            };

            TXPErrorCode returnCode = TXPErrorCode.TXP_RETURN_GENERAL_FAILURE;
            string fullPath = Path.GetFullPath(assetPath);

            ExportImageInfo exportInfo = TxpGetImageInfo(fullPath, fullPath.Length);

            bool getImageInfoFailed = exportInfo.textureFormat == TxpTextureFormat.FMT_UNKNOWN ||
               exportInfo.ImageIOHandler == IntPtr.Zero ||
               exportInfo.width == 0 ||
               exportInfo.height == 0;

            bool isHighP = exportInfo.textureFormat > TxpTextureFormat.FMT_RGBA8;
            bool isHigherRes = (exportInfo.width >= 2 * output.width) && (exportInfo.height >= 2 * output.height);

            bool isGrayscale = inputImport.convertToNormalmap;

            bool readSource = !isGrayscale && (isHighP || (isHigherRes && geoRoughness));
            readSource = readSource && !texSettings.dontReadSource;

            if (!readSource || getImageInfoFailed)
            {
               // Debug.Log($"SLZ TexProc: using unity imported image for {inputImport.assetPath}");
                NativeArray<byte> mip0Copy = new NativeArray<byte>(mip0.Length, Allocator.TempJob);
                NativeArray<byte>.Copy(mip0, 0, mip0Copy, 0, mip0.Length);

                IntPtr mip0CopyPtr = TxpNativeArrayIntPtr.GetIntPtr(mip0Copy);
                returnCode = (TXPErrorCode) TxpProcessNormalMap(
                    mip0CopyPtr, output.width, output.height,
                    expFmt,
                    textureDesc,
                    isDXTnm ? 1 : 0,
                    isDXTnm ? 1 : 0,
                    isDetailMap ? 1 : 0,
                    isHemiOct ? 1 : 0,
                    geoRoughness ? 1 : 0);
                mip0Copy.Dispose();
            }
            else 
            {
                TxpTextureChannelFmt inputChannelFmt = SLZTextureImporter.GetChannelFmtFromTxp(exportInfo.textureFormat);
                TxpTextureChannelFmt outputChannelFmt = SLZTextureImporter.GetChannelFmtFromGfx(output.graphicsFormat);
                if (inputChannelFmt != outputChannelFmt)
                {
                    uint channelCount = GraphicsFormatUtility.GetComponentCount(output.graphicsFormat);
                    GraphicsFormat newGfx = SLZTextureImporter.GetGfxFormatFromChannels(inputChannelFmt, (int)channelCount, false);
                    //Debug.Log($"Original Gfx Fmt: {output.graphicsFormat.ToString()}, New Gfx Fmt: {newGfx.ToString()}");
                    if (newGfx != output.graphicsFormat) output.Reinitialize(output.width, output.height, newGfx, output.mipmapCount > 1);
                    textureDesc.format = SLZTextureImporter.GetFmtForExport(newGfx);

                    for (int mIdx = 0; mIdx < mipCount; mIdx++)
                    {
                        mipChain[mIdx] = TxpNativeArrayIntPtr.GetIntPtr(output.GetPixelData<byte>(mIdx));
                    }
                }
                // Debug.Log($"SLZ TexProc: Attempting to read image for {inputImport.assetPath}");
                returnCode = (TXPErrorCode)TxpReadAndProcessNormalMap(
                    exportInfo,
                    textureDesc,
                    isDetailMap ? 1 : 0, // Only detail maps are DXTnm in the source image
                    isDXTnm ? 1 : 0,
                    isDetailMap ? 1 : 0,
                    isHemiOct ? 1 : 0,
                    geoRoughStr
                    );
            }
            mipResolutions.Dispose();
            mipChain.Dispose();
            TxpDisposeImageInfo(exportInfo);


            if (returnCode != TXPErrorCode.TXP_RETURN_SUCCESS)
            {
                Debug.LogError($"SLZ Texture Post-Processor: Failed to import texture. Error code: {Enum.GetName(typeof(TXPErrorCode), returnCode)}, Path: {inputImport.assetPath}\n");
                //output.Reinitialize(32, 32, output.graphicsFormat, false);
            }
        }


        bool IsDXTnm()
        {
            
            TextureImporter importer = this.assetImporter as TextureImporter;
            TextureFormat outFmt;
            ColorSpace outSpace;
            int compressionQuality;
            importer.ReadTextureImportInstructions(EditorUserBuildSettings.activeBuildTarget, out outFmt, out outSpace, out compressionQuality);

            bool isDTXnm = false;
            if (outFmt == TextureFormat.BC7 || 
                outFmt == TextureFormat.DXT5 ||
                outFmt == TextureFormat.DXT5Crunched) 
            {
                isDTXnm = true;
            }

            return isDTXnm;
        }
    }
}
