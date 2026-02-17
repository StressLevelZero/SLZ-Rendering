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


DLLEXPORT ExportImageInfo STDCALL TxpGetImageInfo(PATH_CHAR * path, int pathLength);
DLLEXPORT void STDCALL TxpDisposeImageInfo(ExportImageInfo exportInfo);
DLLEXPORT int STDCALL TxpReadAndProcessNormalMap(ExportImageInfo exportInfo, TxpTex2D texture, int inDXTnm, int outDXTnm, int isDetail, int isHemiOct, float geoRoughStr);


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
	ExportImageInfo exportInfo = TxpGetImageInfo(path.s, path.length);
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
	//ExportImageInfo* imageInfo = (ExportImageInfo*)exportInfo;
	//ImageIoHandler* io = (ImageIoHandler*)imageInfo->ImageIOHandler;


	printf("Finished processing normal map\n");
	
	
	if (exportInfo.textureFormat == FMT_RGBAFloat)
	{
		for (int mIdx = 0; mIdx < testTex->mipCount; mIdx++)
		{

			ConvertImage_vec4s_fix16v4(testTex->mips[mIdx], testTex->mips[mIdx], testTex->resolution[mIdx]);
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



DLLEXPORT ExportImageInfo STDCALL TxpGetImageInfo(PATH_CHAR* path, int pathLength)
{
	ExportImageInfo exportInfo = {};
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


DLLEXPORT void STDCALL TxpDisposeImageInfo(ExportImageInfo exportInfo)
{

	if (exportInfo.ImageIOHandler != NULL)
	{
		DisposeImageHandler(exportInfo.ImageIOHandler);
		free(exportInfo.ImageIOHandler);
		exportInfo.ImageIOHandler = NULL;
	}
}

DLLEXPORT int STDCALL TxpReadAndProcessNormalMap(ExportImageInfo exportInfo, TxpTex2D texture, int inDXTnm, int outDXTnm, int isDetail, int isHemiOct, float geoRoughStr)
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

DLLEXPORT int STDCALL TxpGenerateThumbnail(TxpTextureFormat formatIn, void* imageIn, int2 resIn, void* imageOut, int2 resOut, ivec4s swizzle, int isNormal, int hemiOct)
{
	return CreateThumbnail(formatIn, imageIn, resIn, imageOut, resOut, swizzle, isNormal, hemiOct);
}