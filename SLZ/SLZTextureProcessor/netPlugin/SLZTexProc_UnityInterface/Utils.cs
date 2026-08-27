using System;
using System.IO;
using System.Collections.Generic;
using UnityEngine;
using static SLZ.SLZTextureProcessor.NativeShared;
using UnityEngine.Experimental.Rendering;
using Unity.Collections;
using Unity.Mathematics;
using UnityEditor;
using System.Runtime.InteropServices;

namespace SLZ.SLZTextureProcessor
{
    /// <summary>
    /// <see cref="UnityEditor.TextureImporterSwizzle"/> extended to contain Half (0.5) as an option
    /// </summary>
    public enum TexImportSwizzleExt
    {
        R           = TextureImporterSwizzle.R,
        G           = TextureImporterSwizzle.G,
        B           = TextureImporterSwizzle.B,
        A           = TextureImporterSwizzle.A,
        OneMinusR   = TextureImporterSwizzle.OneMinusR,
        OneMinusG   = TextureImporterSwizzle.OneMinusG,
        OneMinusB   = TextureImporterSwizzle.OneMinusB,
        OneMinusA   = TextureImporterSwizzle.OneMinusA,
        Zero        = TextureImporterSwizzle.Zero,
        One         = TextureImporterSwizzle.One,
        Half        = 10
    };

    /// <summary>
    /// Affine 4x5 transform matrix for RGBA colors
    /// </summary>
    public struct ColorTransform
    {
        public float4x4 c03;
        public float4 c4;

        public static ColorTransform identity = new ColorTransform()
        {
            c03 = float4x4.identity,
            c4 = float4.zero
        };

        public float4 mul(float4 color)
        {
            return math.mul(c03, color) + c4;
        }
    }

    public static class TxpUtils
    {

        public static GraphicsFormat TxpFormatToGfxFormat(TxpTextureFormat expFmt)
        {
            switch (expFmt)
            {
                case (TxpTextureFormat.FMT_R8       ): return GraphicsFormat.R8_UNorm           ;
                case (TxpTextureFormat.FMT_RG8      ): return GraphicsFormat.R8G8_UNorm         ;
                case (TxpTextureFormat.FMT_RGB8     ): return GraphicsFormat.R8G8B8_UNorm       ;
                case (TxpTextureFormat.FMT_RGBA8    ): return GraphicsFormat.R8G8B8A8_UNorm     ;
                case (TxpTextureFormat.FMT_R16      ): return GraphicsFormat.R16_UNorm          ;
                case (TxpTextureFormat.FMT_RG16     ): return GraphicsFormat.R16G16_UNorm       ;
                case (TxpTextureFormat.FMT_RGB16    ): return GraphicsFormat.R16G16B16_UNorm    ;
                case (TxpTextureFormat.FMT_RGBA16   ): return GraphicsFormat.R16G16B16A16_UNorm ;
                case (TxpTextureFormat.FMT_RHalf    ): return GraphicsFormat.R16_SFloat         ;
                case (TxpTextureFormat.FMT_RGHalf   ): return GraphicsFormat.R16G16_SFloat      ;
                case (TxpTextureFormat.FMT_RGBHalf  ): return GraphicsFormat.R16G16B16_SFloat   ;
                case (TxpTextureFormat.FMT_RGBAHalf ): return GraphicsFormat.R16G16B16A16_SFloat;
                case (TxpTextureFormat.FMT_RFloat   ): return GraphicsFormat.R32_SFloat         ;
                case (TxpTextureFormat.FMT_RGFloat  ): return GraphicsFormat.R32G32_SFloat      ;
                case (TxpTextureFormat.FMT_RGBFloat ): return GraphicsFormat.R32G32B32_SFloat   ;
                case (TxpTextureFormat.FMT_RGBAFloat): return GraphicsFormat.R32G32B32A32_SFloat;
                default: return GraphicsFormat.None;
            }
        }

        public static TxpTextureFormat GfxFormatToTxpFormat(GraphicsFormat gfxFmt)
        {
            switch (gfxFmt)
            {
                case (GraphicsFormat.R8_UNorm           ): return TxpTextureFormat.FMT_R8       ;
                case (GraphicsFormat.R8G8_UNorm         ): return TxpTextureFormat.FMT_RG8      ;
                case (GraphicsFormat.R8G8B8_UNorm       ): return TxpTextureFormat.FMT_RGB8     ;
                case (GraphicsFormat.R8G8B8A8_UNorm     ): return TxpTextureFormat.FMT_RGBA8    ;
                case (GraphicsFormat.R16_UNorm          ): return TxpTextureFormat.FMT_R16      ;
                case (GraphicsFormat.R16G16_UNorm       ): return TxpTextureFormat.FMT_RG16     ;
                case (GraphicsFormat.R16G16B16_UNorm    ): return TxpTextureFormat.FMT_RGB16    ;
                case (GraphicsFormat.R16G16B16A16_UNorm ): return TxpTextureFormat.FMT_RGBA16   ;
                case (GraphicsFormat.R16_SFloat         ): return TxpTextureFormat.FMT_RHalf    ;
                case (GraphicsFormat.R16G16_SFloat      ): return TxpTextureFormat.FMT_RGHalf   ;
                case (GraphicsFormat.R16G16B16_SFloat   ): return TxpTextureFormat.FMT_RGBHalf  ;
                case (GraphicsFormat.R16G16B16A16_SFloat): return TxpTextureFormat.FMT_RGBAHalf ;
                case (GraphicsFormat.R32_SFloat         ): return TxpTextureFormat.FMT_RFloat   ;
                case (GraphicsFormat.R32G32_SFloat      ): return TxpTextureFormat.FMT_RGFloat  ;
                case (GraphicsFormat.R32G32B32_SFloat   ): return TxpTextureFormat.FMT_RGBFloat ;
                case (GraphicsFormat.R32G32B32A32_SFloat): return TxpTextureFormat.FMT_RGBAFloat;
                default: return TxpTextureFormat.FMT_UNKNOWN;
            }
        }


        public static bool FormatHasAlpha(TxpTextureFormat format)
        {
            switch (format)
            {
                case TxpTextureFormat.FMT_RGBA8:
                case TxpTextureFormat.FMT_RGBA16:
                case TxpTextureFormat.FMT_RGBAHalf:
                case TxpTextureFormat.FMT_RGBAFloat:
                    return true;
                default: 
                    return false;
            }
        }

        public struct NativeImage
        {
            public TxpTexSubresource subresource;
            public NativeArray<byte> data;

            public void Dispose()
            {
                data.Dispose();
            }
        }

        public static ColorTransform SwizzleEnumToMatrix(
            TexImportSwizzleExt red,
            TexImportSwizzleExt green,
            TexImportSwizzleExt blue,
            TexImportSwizzleExt alpha)
        {
            ColorTransform output = new ColorTransform();

            SwizzleEnumToRow(red,   out output.c03.c0, out output.c4.x);
            SwizzleEnumToRow(green, out output.c03.c1, out output.c4.y);
            SwizzleEnumToRow(blue,  out output.c03.c2, out output.c4.z);
            SwizzleEnumToRow(alpha, out output.c03.c3, out output.c4.w);
            output.c03 = math.transpose(output.c03);
            return output;
        }

        public static void SwizzleEnumToRow(TexImportSwizzleExt channel, out float4 r03, out float r4)
        {
            switch (channel)
            {
                case TexImportSwizzleExt.R        : r03 = new float4( 1.0f, 0.0f, 0.0f, 0.0f); r4 = 0.0f; return;
                case TexImportSwizzleExt.G        : r03 = new float4( 0.0f, 1.0f, 0.0f, 0.0f); r4 = 0.0f; return;
                case TexImportSwizzleExt.B        : r03 = new float4( 0.0f, 0.0f, 1.0f, 0.0f); r4 = 0.0f; return;
                case TexImportSwizzleExt.A        : r03 = new float4( 0.0f, 0.0f, 0.0f, 1.0f); r4 = 0.0f; return;
                case TexImportSwizzleExt.OneMinusR: r03 = new float4(-1.0f, 0.0f, 0.0f, 0.0f); r4 = 1.0f; return;
                case TexImportSwizzleExt.OneMinusG: r03 = new float4( 0.0f,-1.0f, 0.0f, 0.0f); r4 = 1.0f; return;
                case TexImportSwizzleExt.OneMinusB: r03 = new float4( 0.0f, 0.0f,-1.0f, 0.0f); r4 = 1.0f; return;
                case TexImportSwizzleExt.OneMinusA: r03 = new float4( 0.0f, 0.0f, 0.0f,-1.0f); r4 = 1.0f; return;
                case TexImportSwizzleExt.Zero     : r03 = new float4( 0.0f, 0.0f, 0.0f, 0.0f); r4 = 0.0f; return;
                case TexImportSwizzleExt.One      : r03 = new float4( 0.0f, 0.0f, 0.0f, 0.0f); r4 = 1.0f; return;
                case TexImportSwizzleExt.Half     : r03 = new float4( 0.0f, 0.0f, 0.0f, 0.0f); r4 = 0.5f; return;
                default: throw new NotImplementedException($"Invalid enum {channel} (value: {(int)channel})");
            }
        }

        public static TxpTexSubresource CreateSubresourceFromData<T>(NativeArray<T> data, int width, int height, TxpTextureFormat format) where T :struct
        {
            TxpTexSubresource output = new TxpTexSubresource()
            {
                data = TxpNativeArrayIntPtr.GetIntPtr(data),
                width = width,
                height = height,
                format = format
            };
            return output;
        }

        public static TxpTexSubresource GetSubResource(Texture2D image, int mip = 0)
        {
            NativeArray<byte> data = image.GetPixelData<byte>(mip);
            TxpTexSubresource output = new TxpTexSubresource()
            {
                data = TxpNativeArrayIntPtr.GetIntPtr(data),
                width = image.width,
                height = image.height,
                format = GfxFormatToTxpFormat(image.graphicsFormat)
            };
            return output;
        }

        public static TxpTexSubresource GetSubResource(Texture2DArray image, int mip = 0, int slice = 0)
        {
            NativeArray<byte> data = image.GetPixelData<byte>(mip, slice);
            TxpTexSubresource output = new TxpTexSubresource()
            {
                data = TxpNativeArrayIntPtr.GetIntPtr(data),
                width = image.width,
                height = image.height,
                format = GfxFormatToTxpFormat(image.graphicsFormat)
            };
            return output;
        }

        public static TxpTexSubresource GetSubResource(Cubemap image, int mip, CubemapFace face)
        {
            NativeArray<byte> data = image.GetPixelData<byte>(mip, face);
            TxpTexSubresource output = new TxpTexSubresource()
            {
                data = TxpNativeArrayIntPtr.GetIntPtr(data),
                width = image.width,
                height = image.height,
                format = GfxFormatToTxpFormat(image.graphicsFormat)
            };
            return output;
        }

        public static TxpTexSubresource GetSubResource(CubemapArray image, int mip, CubemapFace face, int slice)
        {
            NativeArray<byte> data = image.GetPixelData<byte>(mip, face, slice);
            TxpTexSubresource output = new TxpTexSubresource()
            {
                data = TxpNativeArrayIntPtr.GetIntPtr(data),
                width = image.width,
                height = image.height,
                format = GfxFormatToTxpFormat(image.graphicsFormat)
            };
            return output;
        }

        public static TxpTexSubresource GetSubResource(Texture3D image, int mip, uint zSlice)
        {
            NativeArray<byte> data = image.GetPixelData<byte>(mip);
            uint bytesPerSlice = GraphicsFormatUtility.ComputeMipmapSize(image.width, image.height, image.graphicsFormat);
            zSlice = math.min(zSlice, (uint)image.depth - 1u);
            uint start = bytesPerSlice * (uint)zSlice;
            NativeArray<byte> slice = data.GetSubArray((int)start, (int)bytesPerSlice);

            TxpTexSubresource output = new TxpTexSubresource()
            {
                data = TxpNativeArrayIntPtr.GetIntPtr(slice),
                width = image.width,
                height = image.height,
                format = GfxFormatToTxpFormat(image.graphicsFormat)
            };
            return output;
        }

        /// <summary>
        /// Gets an ImageFileHandler for a given path. Must be manually disposed via <see cref="UnsafeDisposeImageFileHandler"/> to avoid leaks!
        /// </summary>
        /// <param name="path">Path of the image, may be project-relative</param>
        /// <returns>If the image at the given path could be opened, returns an <see cref="ImageFileHandler"/> with a non-IntPtr.Zero <see cref="ImageFileHandler.ImageIOHandler"/> that can be used to read the image </returns>
        public static ImageFileHandler UnsafeGetImageFileHandler(string path)
        {
            string fullPath = Path.GetFullPath(path);
            if (!File.Exists(fullPath))
            {
                Debug.LogError($"GetImageInfo: invalid path {fullPath}");
                return new ImageFileHandler() { };
            }
            return NativeBindings.TxpGetImageInfo(fullPath, fullPath.Length);
        }

        /// <summary>
        /// Disposes of an ImageFileHandler allocated with <see cref="UnsafeGetImageFileHandler"/>
        /// </summary>
        /// <param name="imageHandler">Handler to dispose of</param>
        /// <returns></returns>
        public static void UnsafeDisposeImageFileHandler(ref ImageFileHandler imageHandler)
        {
            NativeBindings.TxpDisposeImageInfo(imageHandler);
            imageHandler.ImageIOHandler = IntPtr.Zero;
        }

        public static TxpTexSubresource UnsafeFileHandlerReadImage(ImageFileHandler handler)
        {
            int errorCode = (int)TXPErrorCode.TXP_RETURN_GENERAL_FAILURE;
            if (handler.ImageIOHandler != IntPtr.Zero)
            {
                errorCode = (int)TXPErrorCode.TXP_RETURN_SUCCESS;
                errorCode = NativeBindings.TxpReadFile(handler);
                if (errorCode == (int)TXPErrorCode.TXP_RETURN_SUCCESS)
                {
                    TxpTexSubresource subresource = NativeBindings.GetSubresourceFromIO(handler);
                    return subresource;
                }
            }
            Debug.LogError($"Failed To Read Image, error code: {(TXPErrorCode)errorCode}");
            return new TxpTexSubresource() { format = TxpTextureFormat.FMT_UNKNOWN, data = IntPtr.Zero, width = 0, height = 0 };
        }

        public static TXPErrorCode ConvertSwizzleImage(TxpTexSubresource imageIn, TxpTexSubresource imageOut, ColorTransform transform)
        {
            Debug.Assert(imageIn.width == imageOut.width && imageIn.height == imageOut.height);
            int errorCode = NativeBindings.ConvertSwizzleSubresource(imageIn, imageOut, transform.c03, transform.c4);
            if (errorCode != (int)TXPErrorCode.TXP_RETURN_SUCCESS)
            {
                Debug.LogError("Failed to transform image colors");
            }
            return (TXPErrorCode) errorCode;
        }


        public static TXPErrorCode SaveSubresourceToPNG(string path, TxpTexSubresource image, int compressionLevel)
        {
            string fullPath = Path.GetFullPath(path);
            string directory = Path.GetDirectoryName(fullPath);
            bool valid = Directory.Exists(directory);
            if (!valid)
            {
                Debug.LogError($"Cannot save png, directory does not exist: {directory}");
                return TXPErrorCode.TXP_RETURN_INVALID_PATH;
            }
            int errorCode = NativeBindings.TxpSaveImageToPNG(path, image, compressionLevel);
            if (errorCode != (int)TXPErrorCode.TXP_RETURN_SUCCESS)
            {
                Debug.LogError($"Failed to save png, error code: {((TXPErrorCode)errorCode)}");
            }
            return (TXPErrorCode)errorCode;
        }



        public static Texture2D ReadImageToTexture2D(string path, ColorTransform colorTransform, bool mips = false, bool readWrite = false)
        {
            
            Texture2D outTex = null;
            ImageFileHandler fileHandler = default;
            try
            {
                string fullPath = Path.GetFullPath(path);
                bool valid = File.Exists(fullPath);
                if (!valid)
                {
                    Debug.LogError($"Cannot open image, file does not exist: {fullPath}");
                    return null;
                }
                fileHandler = UnsafeGetImageFileHandler(fullPath);
                if (fileHandler.ImageIOHandler == IntPtr.Zero)
                {
                    Debug.LogError($"Cannot open image: {fullPath}");
                    return null;
                }
                TxpTexSubresource sourceImage = UnsafeFileHandlerReadImage(fileHandler);

                if (sourceImage.format == TxpTextureFormat.FMT_UNKNOWN || sourceImage.data == IntPtr.Zero)
                {
                    Debug.LogError($"Failed to read image: {fullPath}");
                    return null;
                }
                // Debug.Log($"sourceImage:\n    data: {sourceImage.data:X8}\n    width: {sourceImage.width}\n    height: {sourceImage.height}\n    format: {sourceImage.format}");

                if (NativeBindings.ImageRequiresYFlip(fileHandler) != 0)
                {
                    //Debug.Log("Y-Flipping image...");
                    NativeBindings.TxpYFlipSubresource(sourceImage);
                }

                GraphicsFormat outGfxFmt = SLZTextureImporter.GetGfxFmtFromExport(fileHandler.textureFormat);
                TxpTextureFormat outTxpFmt = SLZTextureImporter.GetFmtForExport(outGfxFmt);

                TextureCreationFlags outImgFlags =
                    TextureCreationFlags.DontInitializePixels |
                    TextureCreationFlags.DontUploadUponCreate |
                    (mips ? TextureCreationFlags.MipChain : TextureCreationFlags.None);

                Texture2D outTexture = new Texture2D(sourceImage.width, sourceImage.height, outGfxFmt, outImgFlags);
                NativeArray<byte> outTexData = outTexture.GetPixelData<byte>(0);
                TxpTexSubresource outImage = new TxpTexSubresource()
                {
                    data = TxpNativeArrayIntPtr.GetIntPtr(outTexData),
                    width = outTexture.width,
                    height = outTexture.height,
                    format = outTxpFmt
                };


                TXPErrorCode returnCode = ConvertSwizzleImage(sourceImage, outImage, colorTransform);

                if (returnCode != TXPErrorCode.TXP_RETURN_SUCCESS)
                {
                    Debug.LogError($"ReadImageToTexture: failed to transfer source image to texture, error code {returnCode}");
                }

                outTexture.Apply(mips, !readWrite);

                outTex = outTexture;
            }
            finally
            {
                if (fileHandler.ImageIOHandler != IntPtr.Zero) UnsafeDisposeImageFileHandler(ref fileHandler);
            }
            return outTex;
        }

        public static Texture2D ReadImageToTexture2D(
            string path, bool mips = false, bool readWrite = false,
            TexImportSwizzleExt red     = TexImportSwizzleExt.R,
            TexImportSwizzleExt green   = TexImportSwizzleExt.G, 
            TexImportSwizzleExt blue    = TexImportSwizzleExt.B,
            TexImportSwizzleExt alpha   = TexImportSwizzleExt.A
            )
        {
            ColorTransform colorTransform = SwizzleEnumToMatrix(red, green, blue, alpha);
            return ReadImageToTexture2D(path, colorTransform, mips, readWrite);
        }

        public static void SaveTexture2DToPNG(string path, Texture2D image, int mipLevel = 0, int compressionLevel = 4)
        {
            if (image == null ) throw new ArgumentNullException("image");
            mipLevel = math.clamp(mipLevel, 0, image.mipmapCount - 1);
            NativeArray<byte> data = image.GetPixelData<byte>(mipLevel);
            TxpTextureFormat outTxpFmt = SLZTextureImporter.GetFmtForExport(image.graphicsFormat);
            if (
                outTxpFmt != TxpTextureFormat.FMT_R8    &&
                outTxpFmt != TxpTextureFormat.FMT_R16   &&
                outTxpFmt != TxpTextureFormat.FMT_RGB8  &&
                outTxpFmt != TxpTextureFormat.FMT_RGB16 &&
                outTxpFmt != TxpTextureFormat.FMT_RGBA8 &&
                outTxpFmt != TxpTextureFormat.FMT_RGBA16
                )
            {
               throw new Exception($"Cannot save texture with format {image.graphicsFormat} to PNG, only 8 or 16 bit fixed point formats with  R, RGB, or RGBA channels are supported");
            }

            TxpTexSubresource outImage = new TxpTexSubresource()
            {
                data = TxpNativeArrayIntPtr.GetIntPtr(data),
                width = math.max(1, image.width >> mipLevel),
                height = math.max(1, image.height >> mipLevel),
                format = outTxpFmt
            };

            NativeBindings.TxpYFlipSubresource(outImage);

            TXPErrorCode error = SaveSubresourceToPNG(path, outImage, compressionLevel);
            if (error != TXPErrorCode.TXP_RETURN_SUCCESS)
            {
                throw new Exception("Failed to save PNG, error code " + error);
            }
        }

        public static void SaveNativeArrayToPNG<T>(string path, GraphicsFormat dataFormat, NativeArray<T> data, int width, int height, int compressionLevel = 4) where T : struct
        {
            TxpTextureFormat outTxpFmt = GfxFormatToTxpFormat(dataFormat);
            if (
                outTxpFmt != TxpTextureFormat.FMT_R8 &&
                outTxpFmt != TxpTextureFormat.FMT_R16 &&
                outTxpFmt != TxpTextureFormat.FMT_RGB8 &&
                outTxpFmt != TxpTextureFormat.FMT_RGB16 &&
                outTxpFmt != TxpTextureFormat.FMT_RGBA8 &&
                outTxpFmt != TxpTextureFormat.FMT_RGBA16
                )
            {
                throw new Exception($"Cannot save texture with format {dataFormat} to PNG, only 8 or 16 bit fixed point formats with  R, RGB, or RGBA channels are supported");
            }
            SaveNativeArrayToPNG(path, outTxpFmt, data, width, height, compressionLevel);
        }
        public static void SaveNativeArrayToPNG<T>(string path, TxpTextureFormat dataFormat, NativeArray<T> data, int width, int height, int compressionLevel = 4) where T : struct
        {
            if (!data.IsCreated) throw new ArgumentNullException("image");

            int bytesPerPixel = 0;
            switch (dataFormat)
            {
                case TxpTextureFormat.FMT_R8:      bytesPerPixel = 1 ; break;
                case TxpTextureFormat.FMT_R16:     bytesPerPixel = 2 ; break;
                case TxpTextureFormat.FMT_RGB8:    bytesPerPixel = 3 ; break;
                case TxpTextureFormat.FMT_RGB16:   bytesPerPixel = 6 ; break;
                case TxpTextureFormat.FMT_RGBA8:   bytesPerPixel = 4 ; break;
                case TxpTextureFormat.FMT_RGBA16:  bytesPerPixel = 8 ; break;
                default: throw new Exception($"Unhandled texture format " + dataFormat);
            }

            ulong calculatedFileSize = (ulong)bytesPerPixel * (ulong)width * (ulong)height;
            ulong dataSize = (ulong)Marshal.SizeOf<T>() * (ulong)data.Length;

            if (dataSize != calculatedFileSize)
            {
                throw new InvalidDataException($"Size of the data NativeArray does not match the size of image with the given dimensions and graphics format!\n" +
                    $"Image size: {dataFormat} = {bytesPerPixel} bytes per pixel, {bytesPerPixel} * {width} * {height} = {calculatedFileSize}\n" +
                    $"Data size: {nameof(T)} = {Marshal.SizeOf<T>()} bytes, array length = {data.Length}, total size = {dataSize}");
            }

            TxpTexSubresource outImage = new TxpTexSubresource()
            {
                data = TxpNativeArrayIntPtr.GetIntPtr(data),
                width = width,
                height = height,
                format = dataFormat
            };

            NativeBindings.TxpYFlipSubresource(outImage);

            TXPErrorCode error = SaveSubresourceToPNG(path, outImage, compressionLevel);
            if (error != TXPErrorCode.TXP_RETURN_SUCCESS)
            {
                throw new Exception("Failed to save PNG, error code " + error);
            }
        }
    }
}
