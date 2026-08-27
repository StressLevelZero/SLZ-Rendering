#pragma once
#include "Platform.h"
#include "texture.h"

typedef enum ImageType
{
	TXP_IMG_UNSUPPORTED = -1,
	TXP_IMG_INVALID = 0,
	TXP_IMG_PNG = 1,
	TXP_IMG_JPG = 2,
	TXP_IMG_TGA = 3,
	TXP_IMG_BMP = 4,
	TXP_IMG_PSD = 5,
	TXP_IMG_GIF = 6,
	TXP_IMG_HDR = 7,
	TXP_IMG_EXR = 8,
	TXP_IMG_TIFF = 9
} ImageType;

typedef enum ImageLibrary
{
	TXP_IMGLIB_NONE = 0,
	TXP_IMGLIB_SPNG = 1,
	TXP_IMGLIB_STB = 2,
	TXP_IMGLIB_EXR = 3,
	TXP_IMGLIB_SAIL = 4,
	TXP_IMGLIB_TIFF = 5,
} ImageLibrary;

typedef struct ImageIoHandler
{
	ImageLibrary imageLibrary;
	PathString path;
	ImageType type;
	TxpTextureChannelFmt channelFormat;
	// Assume R, RG, RGB, or RGBA. I'm not going to directly store formats with out of order colors like BGRA or single channel plus alpha formats. The image loading methods will have to be responsible for repacking to a sane layout.
	int channelCount; 
	int width;
	int height;
	int depth;
	int mipChain; // 1 if theres a pre-generated mip chain. EXR can actually can contain a mip chain, might support importing uncompressed DDS/KTX formats
	int yFlip; // image needs to be flipped on Y. Unity expects 0,0 to be the bottom left corner of the image
	void* ILD; // Image Library Data, pointer to struct containing library-dependent information
	void* imageBuffer;
} ImageIoHandler;

int InitImageHandler(const PathString* path, ImageIoHandler* handler);
void DisposeImageHandler(ImageIoHandler* handler);

TXPErrorCode IOReadImage(ImageIoHandler* imageIO);
void DisposeImageBuffer(ImageIoHandler* imageIO);

TXPErrorCode GetImageInfo(ImageIoHandler* imageIO);

void DebugTestSpng(TxpTex2D* outTex);
TXPErrorCode SaveToPng(PATH_CHAR* savePath, void* imageBuffer, int width, int height, TxpTextureFormat pixelFormat, int compressionLevel);
