#include "Platform.h"
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include <locale.h>

#include "imageIO.h"
#define SAIL_STATIC 
//#include <sail/sail.h>
//#include "spng.h"
#include "main_openmp.h"

// Continue in a loop if a function fails, printing an error message at the same time 
#define SLZ_CONTINUE_ON_FAIL_MSG(rcode, message) if (rcode != 0) { printf message; continue; }
#define SLZ_RETURN_ON_FAIL_MSG(rcode, message) if (rcode != 0) { printf message; return rcode; }


DLLEXPORT ImageFileHandler STDCALL TxpGetImageInfo(PATH_CHAR * path, int pathLength);
DLLEXPORT void STDCALL TxpDisposeImageInfo(ImageFileHandler exportInfo);
DLLEXPORT int STDCALL TxpReadAndProcessNormalMap(ImageFileHandler exportInfo, TxpTex2D texture, int inDXTnm, int outDXTnm, int isDetail, int isHemiOct, float geoRoughStr);


DLLEXPORT void STDCALL TxpSetLogger(void* fnPtr)
{
    LogFn = fnPtr;
    //DebugLog(UNITY_LOG_LEVEL_LOG, "Bound texture processor logger\n");
}

DLLEXPORT int STDCALL TestImageRead()
{
    InitializeLibPath();
    PrintLibPath();

    //sail_set_logger(fake_sail_logger);

    //PATH_CHAR imageName[] = PATH_LITERAL("Б.png");
    PATH_CHAR imageName[] = PATH_LITERAL("TestImg.png");
    PATH_CHAR imagePath[g_libraryPath->length + (sizeof(imageName) / sizeof(PATH_CHAR))];

    memcpy(imagePath, g_libraryPath->s, g_libraryPath->length * sizeof(PATH_CHAR));
    memcpy(imagePath + g_libraryPath->length, imageName, sizeof(imageName));

    char* shortPath = GetPlatShortPath(imagePath);
    printf("Short path %s\n", shortPath);
#ifdef PLATFORM_WINDOWS
    PATH_CHAR* longPath = UTF8ToUTF16(shortPath);
    wprintf(L"Long Path %s\n", longPath);
#endif
    FreePlatShortPath(shortPath);
    PathString path = {
        .length = g_libraryPath->length + (sizeof(imageName) / sizeof(PATH_CHAR)) - 1,
        .s = imagePath
    };
    DebugLog(0, "TestLog\n");
    ImageFileHandler exportInfo = TxpGetImageInfo(path.s, path.length);
    printf("exportInfo texture format: %d\n", exportInfo.textureFormat);
    if (exportInfo.textureFormat == FMT_UNKNOWN)
    {
        printf("Failed to get export info\n");
        TxpDisposeImageInfo(exportInfo);
        return 1;
    }

    
    TxpTex2D* testTex;
    TXPErrorCode err = CreateTexture2D(&testTex, exportInfo.width, exportInfo.height, exportInfo.textureFormat, 0);

    int intErr = TxpReadAndProcessNormalMap(exportInfo, *testTex, 0, 0, 0, 0, 0);
    printf("Return code: %d\n", intErr);
    //ImageFileHandler* imageInfo = (ImageFileHandler*)exportInfo;
    //ImageIoHandler* io = (ImageIoHandler*)imageInfo->ImageIOHandler;


    printf("Finished processing normal map\n");
    
    
    if (exportInfo.textureFormat == FMT_RGBAFloat)
    {
        for (int mIdx = 0; mIdx < testTex->mipCount; mIdx++)
        {

            ConvertImage_float4_fix16v4(testTex->mips[mIdx], testTex->mips[mIdx], testTex->resolution[mIdx]);
            testTex->format = FMT_RGBA16;
        }
    }

    if (exportInfo.textureFormat == FMT_RGBAHalf)
    {
        for (int mIdx = 0; mIdx < testTex->mipCount; mIdx++)
        {
            ConvertImage_half4_fix16v4(testTex->mips[mIdx], testTex->mips[mIdx], testTex->resolution[mIdx]);
            testTex->format = FMT_RGBA16;
        }
    }
    printf("Finished converting normal map\n");
    TxpDisposeImageInfo(exportInfo);

    DebugTestSpng(testTex);
    DisposeTexture2D(testTex);
    return 0;
}



DLLEXPORT ImageFileHandler STDCALL TxpGetImageInfo(PATH_CHAR* path, int pathLength)
{
    ImageFileHandler exportInfo = {};
    PathString pathStr = { .length = pathLength, .s = path };
    
    ImageIoHandler* imageHandler = malloc(sizeof(ImageIoHandler));
    *imageHandler = (ImageIoHandler){};
    TXPErrorCode error = InitImageHandler(&pathStr, imageHandler);
    if (error != TXP_RETURN_SUCCESS )
    {
        if (error != TXP_RETURN_NO_IMG_LIBRARY)
        {
            char* pathUTF8 = GetPlatShortPath(path);
            const char* errorStr = TXPErrorToStr(error);
            DebugLog(UNITY_LOG_LEVEL_ERROR, "SLZ TexProc: TxpGetImageInfo failed to initialize image handler with error %s, path: %s\n", errorStr, pathUTF8);
            FreePlatShortPath(pathUTF8);
        }
        DisposeImageHandler(imageHandler);
        free(imageHandler);
        imageHandler = NULL;
        return exportInfo;
    }
    error = GetImageInfo(imageHandler);
    if (error != TXP_RETURN_SUCCESS)
    {
        char* pathUTF8 = GetPlatShortPath(path);
        const char* errorStr = TXPErrorToStr(error);
        DebugLog(UNITY_LOG_LEVEL_ERROR, "SLZ TexProc: TxpGetImageInfo failed to get image properties with error %s, path: %s\n", errorStr, pathUTF8);
        FreePlatShortPath(pathUTF8);
        DisposeImageHandler(imageHandler);
        free(imageHandler);
        imageHandler = NULL;
        return exportInfo;
    }
    exportInfo.width = imageHandler->width;
    exportInfo.height = imageHandler->height;
    exportInfo.textureFormat = TextureFormatFromChannelInfo(imageHandler->channelFormat, imageHandler->channelCount);
    exportInfo.ImageIOHandler = imageHandler;
    return exportInfo;
}

DLLEXPORT TxpTexSubresource GetSubresourceFromIO(ImageFileHandler info)
{
    TxpTexSubresource output = {};
    ImageIoHandler* io = (ImageIoHandler*)info.ImageIOHandler;
    if (io != NULL && io->imageBuffer != NULL)
    {
        output.data = io->imageBuffer;
        output.width = io->width;
        output.height = io->height;
        output.format = TextureFormatFromChannelInfo(io->channelFormat, io->channelCount);
    }
    return output;
}

DLLEXPORT int ImageRequiresYFlip(ImageFileHandler info)
{
    ImageIoHandler* io = (ImageIoHandler*)info.ImageIOHandler;
    if (io != NULL)
    {
        return io->yFlip;
    }
    return 0;
}


DLLEXPORT void STDCALL TxpDisposeImageInfo(ImageFileHandler exportInfo)
{

    if (exportInfo.ImageIOHandler != NULL)
    {
        DisposeImageHandler(exportInfo.ImageIOHandler);
        free(exportInfo.ImageIOHandler);
        exportInfo.ImageIOHandler = NULL;
    }
}

DLLEXPORT int STDCALL TxpReadAndProcessNormalMap(ImageFileHandler exportInfo, TxpTex2D texture, int inDXTnm, int outDXTnm, int isDetail, int isHemiOct, float geoRoughStr)
{
    TXPErrorCode err = TXP_RETURN_SUCCESS;
    ImageIoHandler* io = (ImageIoHandler*)exportInfo.ImageIOHandler;
    err = IOReadImage(io);
    if (err)
    {
        DebugLog(UNITY_LOG_LEVEL_ERROR, "Failed to Read image, error code %s\n", TXPErrorToStr(err));
        goto exit;
    }
    //printf("Output Texture dim: %d x %d\n", texture.resolution[0].x, texture.resolution[0].y);

    ivec4s inSwizzle = isDetail || inDXTnm ? (ivec4s) { 3, 1, 2, 0 } : (ivec4s) { 0, 1, 2, 3 };
    ivec4s outSwizzle = isDetail || outDXTnm ? (ivec4s) { 3, 1, 2, 0 } : (ivec4s) { 0, 1, 2, 3 };

    err = ProcessNormalMap(
        exportInfo.textureFormat, texture.format,
        io->imageBuffer, 
        (int2){exportInfo.width, exportInfo.height},
        &texture, 
        inSwizzle, outSwizzle, 
        io->yFlip,
        isDetail, 
        isHemiOct, 
        geoRoughStr, 
        texture.mipCount > 1);
exit:
    return (int)err;
}

DLLEXPORT int STDCALL TxpReadFile(ImageFileHandler exportInfo)
{
    TXPErrorCode err = TXP_RETURN_SUCCESS;
    ImageIoHandler* io = (ImageIoHandler*)exportInfo.ImageIOHandler;
    err = IOReadImage(io);
    if (err)
    {
        DebugLog(UNITY_LOG_LEVEL_ERROR, "Failed to Read image, error code %s\n", TXPErrorToStr(err));
    }
    return (int)err;
}

DLLEXPORT int STDCALL TxpYFlipSubresource(TxpTexSubresource texture)
{
    TXPErrorCode err = TXP_RETURN_SUCCESS;
   

    err = YFlipImage(texture.data, TextureFormatSize(texture.format) * texture.width, texture.height);
    if (err)
    {
        DebugLog(UNITY_LOG_LEVEL_ERROR, "Failed to Y-Flip MAS image, error code %s\n", TXPErrorToStr(err));
    }
    return (int)err;
}

DLLEXPORT int STDCALL ConvertSwizzleSubresource(TxpTexSubresource textureIn, TxpTexSubresource textureOut, float4x4_unaligned swizzleMatrix, float4_unaligned swizzleAdd)
{
    TXPErrorCode err = TXP_RETURN_SUCCESS;

    if (textureIn.width != textureOut.width || textureIn.height != textureOut.height)
    {
        DebugLog(UNITY_LOG_LEVEL_ERROR, "Cannot Convert/Swizzle texture, input and output dimensions do not match (input: %d x %d, output: %d x %d)\n", textureIn.width, textureIn.height, textureOut.width, textureOut.height);
        return (int)TXP_RETURN_INVALID_ARGS;
    }
    if (textureIn.data == NULL)
    {
        DebugLog(UNITY_LOG_LEVEL_ERROR, "Cannot Convert/Swizzle texture, input subresource has NULL data\n");
        return (int)TXP_RETURN_INVALID_ARGS;
    }
    if (textureOut.data == NULL)
    {
        DebugLog(UNITY_LOG_LEVEL_ERROR, "Cannot Convert/Swizzle texture, output subresource has NULL data\n");
        return (int)TXP_RETURN_INVALID_ARGS;
    }

    /*
    size_t dataOffset = offsetof(TxpTexSubresource, data);
    size_t formOffset = offsetof(TxpTexSubresource, format);
    size_t widtOffset = offsetof(TxpTexSubresource, width);
    size_t heigOffset = offsetof(TxpTexSubresource, height);
    size_t totalSize = sizeof(TxpTexSubresource);
    if (
        dataOffset != 0  ||
        formOffset != 8  ||
        widtOffset != 12 ||
        heigOffset != 16 ||
        totalSize != 32
        )
    {
        DebugLog(UNITY_LOG_LEVEL_ERROR, "Alignments wrong:\n dataOffset %d\nformOffset %d\nwidtOffset %d\nheigOffset %d\ntotalSize %d",
            dataOffset,
            formOffset,
            widtOffset,
            heigOffset,
            totalSize 
        );
    }
    DebugLog(UNITY_LOG_LEVEL_ERROR, "textureIn:\n    data %x (%x) \n    format: %d\n    width: %d\n    height: %d", textureIn.data, ((unsigned int*)textureIn.data)[0], textureIn.format, textureIn.width, textureIn.height);
    DebugLog(UNITY_LOG_LEVEL_ERROR, "textureOut:\n    data %x (%x) \n    format: %d\n    width: %d\n    height: %d", textureOut.data, ((unsigned int*)textureOut.data)[0], textureOut.format, textureOut.width, textureOut.height);
    DebugLog(UNITY_LOG_LEVEL_ERROR, "ConvertSwizzleImage pointer %x", (void*)ConvertSwizzleImage);
    */

    mat4s matrixAligned = {};
    memcpy(&matrixAligned, &swizzleMatrix, sizeof(float4x4_unaligned));
    vec4s addAligned = {};
    memcpy(&addAligned, &swizzleAdd, sizeof(float4_unaligned));

    TxpTextureFormat formatIn = textureIn.format;
    void* dataIn = textureIn.data;
    int2 size = { textureIn.width, textureIn.height };

    err = ConvertSwizzleImage(textureIn.format, textureIn.data, textureOut.format, textureOut.data, size, matrixAligned, addAligned);


    return (int)err;
}

DLLEXPORT int STDCALL TxpSaveImageToPNG(PATH_CHAR* savePath, TxpTexSubresource image, int compressionLevel)
{
    TXPErrorCode err = TXP_RETURN_SUCCESS;
    if (
            image.format != FMT_R8    &&
            image.format != FMT_R16   &&
            image.format != FMT_RGB8  &&
            image.format != FMT_RGBA8 &&
            image.format != FMT_RGB16 &&
            image.format != FMT_RGBA16
        )
    {
        DebugLog(UNITY_LOG_LEVEL_ERROR, "Texture with format %d cannot be encoded to PNG, texture format is not supported. Only 8 or 16 bit fixed point R, RGB, or RGBA images are allowed\n", image.format);
        return TXP_RETURN_INVALID_TEX_FORMAT;
    }
    err = SaveToPng(savePath, image.data, image.width, image.height, image.format, compressionLevel);
    return (int)err;
}

DLLEXPORT int STDCALL TxpProcessNormalMap(void* texInBuffer, int texInWidth, int texInHeight, TxpTextureFormat texInFmt, TxpTex2D texOut, int inDXTnm, int outDXTnm, int isDetail, int isHemiOct, float geoRoughStr)
{
    //printf("Output Texture dim: %d x %d\n", texture.resolution[0].x, texture.resolution[0].y);

    ivec4s inSwizzle = isDetail || inDXTnm ? (ivec4s) { 3, 1, 2, 0 } : (ivec4s) { 0, 1, 2, 3 };
    ivec4s outSwizzle = isDetail || outDXTnm ? (ivec4s) { 3, 1, 2, 0 } : (ivec4s) { 0, 1, 2, 3 };

    TXPErrorCode err = ProcessNormalMap(
        texInFmt, texOut.format,
        texInBuffer,
        (int2) {texInWidth, texInHeight},
        &texOut,
        inSwizzle, outSwizzle, 
        0, // Unity always gives us the correct orientation
        isDetail, 
        isHemiOct, 
        geoRoughStr, 
        texOut.mipCount > 1);
    return (int)err;
}

DLLEXPORT int STDCALL TxpReadAndProcessAoSmNormalMap(
    ImageFileHandler nrmExportInfo,
    ImageFileHandler masExportInfo,
    TxpTex2D texOut, 
    int4_unaligned swizzleNormal_u,
    int4_unaligned swizzleMAS_u,
    int inDXTnm, int outDXTnm, int isDetail, int isHemiOct, float geoRoughStr)
{
    //printf("Output Texture dim: %d x %d\n", texture.resolution[0].x, texture.resolution[0].y);
    ivec4s swizzleNormal = {}; memcpy(&swizzleNormal, &swizzleNormal_u, sizeof(int4_unaligned));
    ivec4s swizzleMAS = {}; memcpy(&swizzleMAS, &swizzleMAS_u, sizeof(int4_unaligned));

    TxpTex2D* combinedBuffer = NULL;

    TXPErrorCode err = TXP_RETURN_SUCCESS;
    ImageIoHandler* nrmIo = (ImageIoHandler*)nrmExportInfo.ImageIOHandler;
    err = IOReadImage(nrmIo);
    if (err)
    {
        DebugLog(UNITY_LOG_LEVEL_ERROR, "Failed to Read Normalmap image, error code %s\n", TXPErrorToStr(err));
        goto exit;
    }
    int2 nrmResIn = (int2){ nrmExportInfo.width, nrmExportInfo.height };
    if (!IsTexImportSwizzleIdentity(swizzleNormal))
    {
        TransformMatrix nrmTransform = TexImportSwizzleToTransform(swizzleNormal);
        TransformInPlace(nrmExportInfo.textureFormat, nrmIo->imageBuffer, nrmResIn, nrmTransform.rotScale, nrmTransform.offset);
    }

    ImageIoHandler* masIo = (ImageIoHandler*)masExportInfo.ImageIOHandler;
    err = IOReadImage(masIo);
    if (err)
    {
        DebugLog(UNITY_LOG_LEVEL_ERROR, "Failed to Read MAS image, error code %s\n", TXPErrorToStr(err));
        goto exit;
    }
    int2 masResIn = (int2){ masExportInfo.width, masExportInfo.height };
    if (!IsTexImportSwizzleIdentity(swizzleMAS))
    {
        TransformMatrix masTransform = TexImportSwizzleToTransform(swizzleMAS);
        //DebugLog(UNITY_LOG_LEVEL_ERROR, "Transform Matrix swizzle: %d %d %d %d:\n"
        //    "%.2f %.2f %.2f %.2f %.2f\n"
        //    "%.2f %.2f %.2f %.2f %.2f\n"
        //    "%.2f %.2f %.2f %.2f %.2f\n"
        //    "%.2f %.2f %.2f %.2f %.2f\n",
        //    swizzleMAS.x, swizzleMAS.y, swizzleMAS.z, swizzleMAS.w,
        //    masTransform.rotScale.raw[0][0], masTransform.rotScale.raw[1][0], masTransform.rotScale.raw[2][0], masTransform.rotScale.raw[3][0], masTransform.offset.raw[0],
        //    masTransform.rotScale.raw[0][1], masTransform.rotScale.raw[1][1], masTransform.rotScale.raw[2][1], masTransform.rotScale.raw[3][1], masTransform.offset.raw[1],
        //    masTransform.rotScale.raw[0][2], masTransform.rotScale.raw[1][2], masTransform.rotScale.raw[2][2], masTransform.rotScale.raw[3][2], masTransform.offset.raw[2],
        //    masTransform.rotScale.raw[0][3], masTransform.rotScale.raw[1][3], masTransform.rotScale.raw[2][3], masTransform.rotScale.raw[3][3], masTransform.offset.raw[3]
        //    );
        TransformInPlace(masExportInfo.textureFormat, masIo->imageBuffer, masResIn, masTransform.rotScale, masTransform.offset);
    }

    //SwizzleInPlace(masExportInfo.textureFormat, masIo->imageBuffer, masResIn, (ivec4s) { 1, 2, 0, 0 });

    if (nrmIo->yFlip != masIo->yFlip)
    {
        err = YFlipImage(masIo->imageBuffer, ChannelFmtToBytes(masIo->channelFormat) * masIo->channelCount * masIo->width, masIo->height);
        if (err)
        {
            DebugLog(UNITY_LOG_LEVEL_ERROR, "Failed to Y-Flip MAS image, error code %s\n", TXPErrorToStr(err));
            goto exit;
        }
    }
    
    TxpTextureFormat combined4WideFmt = To4WideTextureFormat(nrmExportInfo.textureFormat);
   

    err = CreateTexture2D(&combinedBuffer, nrmResIn.x, nrmResIn.y, combined4WideFmt, 0);
    if (err)
    {
        DebugLog(UNITY_LOG_LEVEL_ERROR, "Failed to create resize buffer for MAS, error code %s\n", TXPErrorToStr(err));
        goto exit;
    }

    err = ScaleImageMitchell(masExportInfo.textureFormat, masIo->imageBuffer, masResIn, combined4WideFmt, combinedBuffer->mips[0], nrmResIn);
    if (err)
    {
        DebugLog(UNITY_LOG_LEVEL_ERROR, "Failed to resize MAS texture, error code %s\n", TXPErrorToStr(err));
        goto exit;
    }

    err = CombineAoSmNrm(combined4WideFmt, combinedBuffer->mips[0], nrmExportInfo.textureFormat, nrmIo->imageBuffer, nrmResIn);
    if (err)
    {
        DebugLog(UNITY_LOG_LEVEL_ERROR, "Failed to combine mas and normal textures, error code %s\n", TXPErrorToStr(err));
        goto exit;
    }

    err = ScaleImageMitchell(combined4WideFmt, combinedBuffer->mips[0], nrmResIn, texOut.format, texOut.mips[0], texOut.resolution[0]);
    //DisposeTexture2D(combinedBuffer);
    //masResizeBuffer = NULL;
    //printf("Output Texture dim: %d x %d\n", texture.resolution[0].x, texture.resolution[0].y);

    ivec4s inSwizzle  = (ivec4s) { 3, 1, 2, 0 };
    ivec4s outSwizzle = (ivec4s) { 3, 1, 2, 0 };
    
    err = ProcessNormalMap(
        combined4WideFmt, texOut.format,
        combinedBuffer->mips[0],
        nrmResIn,
        & texOut,
        inSwizzle, outSwizzle,
        nrmIo->yFlip,
        /*isDetail*/2,
        isHemiOct,
        geoRoughStr,
        texOut.mipCount > 1);

exit:
    if (combinedBuffer != NULL) DisposeTexture2D(combinedBuffer);
    return (int)err;
}

DLLEXPORT int STDCALL TxpGenerateThumbnail(TxpTextureFormat formatIn, void* imageIn, int2 resIn, void* imageOut, int2 resOut, ivec4s swizzle, int isNormal, int hemiOct)
{
    return CreateThumbnail(formatIn, imageIn, resIn, imageOut, resOut, swizzle, isNormal, hemiOct);
}