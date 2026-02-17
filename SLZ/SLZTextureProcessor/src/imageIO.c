#include "Platform.h"
#include "texture.h"
#include "imageIO.h"
#include <stdint.h>
#include <stdio.h>
#include <ctype.h>

#include "spng.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_WINDOWS_UTF8
#define STBI_NO_PNG
#define STBI_NO_PIC
#define STBI_NO_PNM 
#include "stb_image.h"

#include "tinyexr_fp16.h"

#define EXT_INT(c1,c2,c3,c4) ((uint32_t)c1 | ((uint32_t)c2 << 8) | ((uint32_t)c3 << 16) | ((uint32_t)c4 << 24))

// Libraries used for each file type

#define LIB_PNG TXP_IMGLIB_SPNG
#define LIB_JPG TXP_IMGLIB_STB
#define LIB_TGA TXP_IMGLIB_STB
#define LIB_BMP TXP_IMGLIB_STB
#define LIB_PSD TXP_IMGLIB_NONE
#define LIB_GIF TXP_IMGLIB_STB
#define LIB_HDR TXP_IMGLIB_STB
#define LIB_EXR TXP_IMGLIB_EXR
#define LIB_TIF TXP_IMGLIB_NONE //IMGLIB_TIFF

static ImageType GetFileTypeFromExt(const PathString* path)
{
	if (path->length < 4)
	{
		return TXP_IMG_INVALID;
	}
	uint32_t extName = 0;
	int maxPath = path->length - 4;
	for (int cIdx = 3; cIdx >= 0; cIdx -= 1)
	{
		uint32_t extChar = tolower((unsigned char)path->s[maxPath + cIdx]);
		if (extChar == '.') break;
		extName = extName | (extChar << (8 * cIdx));
		//printf("Ext char %c, char value %d, total value: %u, cIdx: %d\n", extChar, extChar, extName, cIdx);
	}
	//printf("\nExtentsion as uint: %u, as char: %c%c%c%c\nExpected: %u\n", extName, extName & 0xFF, (extName >> 8) & 0xFF, (extName >> 16) & 0xFF, (extName >> 24) & 0xFF, EXT_INT('\0', 'p', 'n', 'g'));
	switch (extName)
	{
	case EXT_INT('\0', 'p', 'n', 'g'): return TXP_IMG_PNG;
	case EXT_INT('\0', 'j', 'p', 'g'): return TXP_IMG_JPG;
	case EXT_INT('\0', 't', 'g', 'a'): return TXP_IMG_TGA;
	case EXT_INT('\0', 'b', 'm', 'p'): return TXP_IMG_BMP;
	case EXT_INT('\0', 'p', 's', 'd'): return TXP_IMG_PSD;
	case EXT_INT('\0', 'g', 'i', 'f'): return TXP_IMG_GIF;
	case EXT_INT('\0', 'h', 'd', 'r'): return TXP_IMG_HDR;
	case EXT_INT('\0', 'e', 'x', 'r'): return TXP_IMG_EXR;

	case EXT_INT('\0', 't', 'i', 'f'):
	case EXT_INT( 't', 'i', 'f', 'f'): return TXP_IMG_TIFF;

	case EXT_INT('\0', 'j', 'p', 'e'):
	case EXT_INT('\0', 'j', 'i', 'f'):
	case EXT_INT('j', 'p', 'e', 'g'):
	case EXT_INT('j', 'f', 'i', 'f'): return TXP_IMG_JPG;

	default:
		return TXP_IMG_INVALID;
	}
}


typedef union magic_num 
{
	uint32_t number;
	uint8_t bytes[4];
} magic_num;

// ASSUMES LITTLE ENDIAN!
#define MAGIC_NUMBER(b0,b1,b2,b3) ((b0) | (b1 << 8) | (b2 << 16) | (b3 << 24))

#define magic_png  MAGIC_NUMBER(0x89,  'P',  'N',  'G')
#define magic_jpg0 MAGIC_NUMBER(0xFF, 0xD8, 0xFF, 0xDB)
#define magic_jpg1 MAGIC_NUMBER(0xFF, 0xD8, 0xFF, 0xE0)
#define magic_jpg2 MAGIC_NUMBER(0xFF, 0xD8, 0xFF, 0xEE)
#define magic_jpg3 MAGIC_NUMBER(0xFF, 0xD8, 0xFF, 0xE1)
#define magic_exr  MAGIC_NUMBER(0x76, 0x2F, 0x31, 0x01)
#define magic_tif0 MAGIC_NUMBER(0x49, 0x49, 0x2A, 0x00)
#define magic_tif1 MAGIC_NUMBER(0x4D, 0x4D, 0x00, 0x2A)
#define magic_psd  MAGIC_NUMBER(0x38, 0x42, 0x50, 0x53)
#define magic_hdr  MAGIC_NUMBER(0x23, 0x3F, 0x52, 0x41)

static ImageType GetFileTypeFromMagicNumber(FILE* file)
{
	magic_num buffer = {};
	size_t readBytes = fread((char*)buffer.bytes, sizeof(magic_num), 1, file);
	switch (buffer.number)
	{
	case (magic_png):  return TXP_IMG_PNG;
	case (magic_jpg0):
	case (magic_jpg1):
	case (magic_jpg2):
	case (magic_jpg3): return TXP_IMG_JPG;
	case (magic_exr):  return TXP_IMG_EXR;
	case (magic_tif0):
	case (magic_tif1): return TXP_IMG_TIFF;
	case (magic_psd): return TXP_IMG_PSD;
	case (magic_hdr): return TXP_IMG_HDR;
	}
	return TXP_IMG_INVALID;
}

#undef magic_png 
#undef magic_jpg0
#undef magic_jpg1
#undef magic_jpg2
#undef magic_jpg3
#undef magic_exr 
#undef magic_tif0
#undef magic_tif1
#undef magic_psd
#undef magic_hdr

static ImageLibrary GetLibraryForType(ImageType type)
{
	switch (type)
	{
	case (TXP_IMG_INVALID): return TXP_IMGLIB_NONE;
	case (TXP_IMG_PNG): return LIB_PNG;

	// stb_image.h
	case (TXP_IMG_JPG): return LIB_JPG;
	case (TXP_IMG_TGA): return LIB_TGA;
	case (TXP_IMG_BMP): return LIB_BMP;
	case (TXP_IMG_PSD): return LIB_PSD;
	case (TXP_IMG_GIF): return LIB_GIF;
	case (TXP_IMG_HDR): return LIB_HDR;

	case (TXP_IMG_EXR): return LIB_EXR;

	case (TXP_IMG_TIFF): return LIB_TIF;

	default: return TXP_IMGLIB_NONE;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// Create Library Data
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

typedef struct spng_data
{
	FILE* file;
	spng_ctx* ctx;
	int requiresRemapping;
} spng_data;

typedef struct stb_data
{
	char* utf8path;
} stb_data;

typedef struct texr_data
{
	char* utf8path;
	int isHalf;
	const char** layerNames;
} texr_data;


static TXPErrorCode GetLibraryFileDataSPNG(void** ild, const PathString* path)
{
	TXPErrorCode err = TXP_RETURN_SUCCESS;

	FILE* file;
	FILE_OPEN(file, path->s, PATH_LITERAL("rb"));

	if (file == NULL)
	{
		err = TXP_RETURN_FILE_OPEN_FAILED;
		goto printErr;
	}

	spng_data* data = (spng_data*)malloc(sizeof(spng_data));
	if (data == NULL) { err = TXP_RETURN_ALLOC_FAILED; goto exit; }
	*data = (spng_data){};
	data->file = file;
	data->ctx = spng_ctx_new(0);
	int spng_error = spng_set_crc_action(data->ctx, SPNG_CRC_USE, SPNG_CRC_USE);
	if (spng_error)
	{
		DebugLog(UNITY_LOG_LEVEL_ERROR, "SPNG Data: error when setting crc action: %s\n", spng_strerror(spng_error));
		goto printErr;
	}

	size_t limit = 4096 * 4096 * 64;
	spng_set_chunk_limits(data->ctx, limit, limit);
	spng_set_png_file(data->ctx, file);
	*ild = (void*)data;
printErr:
	if (err != TXP_RETURN_SUCCESS)
	{
		char* utf8path = GetPlatShortPath(path->s);
		DebugLog(UNITY_LOG_LEVEL_ERROR, "SPNG Data: failed to open file at path: %s\n", utf8path);
		FreePlatShortPath(utf8path);
	}
exit:
	return err;
}

static TXPErrorCode GetLibraryFileDataSTB(void** ild, const PathString* path)
{
	//FILE* file;
	//errno_t error = FILE_OPEN(&file, path->s, PATH_LITERAL("rb"));
	//if (error)
	//{
	//	printf("STB Data: failed to open file, error code: %d, path: ", error);
	//	PATH_PRINT(path->s);
	//	printf("\n");
	//	return NULL;
	//}
	TXPErrorCode err = TXP_RETURN_SUCCESS;
	stb_data* data = (stb_data*)malloc(sizeof(stb_data));
	if (data == NULL) { err = TXP_RETURN_ALLOC_FAILED; goto exit; }
	*data = (stb_data){};
	data->utf8path = GetPlatShortPath(path->s);
	*ild = (void*)data;
exit:
	return err;
}

static TXPErrorCode GetLibraryFileDataTinyEXR(void** ild, const PathString* path)
{
	TXPErrorCode err = TXP_RETURN_SUCCESS;
	texr_data* data = (texr_data*)malloc(sizeof(texr_data));
	if (data == NULL) { err = TXP_RETURN_ALLOC_FAILED; goto exit; }
	*data = (texr_data){};
	data->isHalf = 0;
	data->layerNames = NULL;
	data->utf8path = GetPlatShortPath(path->s);
	*ild = (void*)data;
exit:
	return err;
}

TXPErrorCode GetLibraryFileData(void** ild, ImageLibrary lib, const PathString* path)
{
	TXPErrorCode err = TXP_RETURN_INVALID_IMG_LIBRARY;
	switch (lib)
	{
	case (TXP_IMGLIB_SPNG): err = GetLibraryFileDataSPNG(ild, path); break;
	case (TXP_IMGLIB_STB):  err = GetLibraryFileDataSTB(ild, path); break;
	case (TXP_IMGLIB_EXR):  err = GetLibraryFileDataTinyEXR(ild, path); break;
	default: 
		DebugLog(UNITY_LOG_LEVEL_ERROR, "SLZ TexProc: GetLibraryFileData got bad image library enum %d", lib);
		break;
	}
	return err;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// Dispose Library Data
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void DisposeLibraryFileDataSPNG(ImageIoHandler* imageIO)
{
	spng_data* data = (spng_data*)imageIO->ILD;
	if (data != NULL)
	{
		if (data->ctx != NULL) spng_ctx_free(data->ctx);
		if (data->file != NULL) fclose(data->file);
		free(data);
	}
	imageIO->ILD = NULL;
}

static void DisposeLibraryFileDataSTB(ImageIoHandler* imageIO)
{
	stb_data* data = (stb_data*)imageIO->ILD;
	if (data != NULL)
	{
		if (data->utf8path != NULL) FreePlatShortPath(data->utf8path);
		free(data);
	}
	imageIO->ILD = NULL;
}

static void DisposeLibraryFileDataTinyEXR(ImageIoHandler* imageIO)
{
	texr_data* data = (texr_data*)imageIO->ILD;
	if (data != NULL)
	{
		if (data->utf8path != NULL) FreePlatShortPath(data->utf8path);
		if (data->layerNames != NULL) TinyEXRFreeMemory(data->layerNames);
		free(data);
	}
	imageIO->ILD = NULL;
}

void DisposeLibraryFileData(ImageIoHandler* imageIO)
{
	switch (imageIO->imageLibrary)
	{
	case (TXP_IMGLIB_SPNG): DisposeLibraryFileDataSPNG(imageIO); break;
	case (TXP_IMGLIB_STB) : DisposeLibraryFileDataSTB(imageIO);  break;
	case (TXP_IMGLIB_EXR): DisposeLibraryFileDataTinyEXR(imageIO); break;
	default: break;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// Get Image Info
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------


static TXPErrorCode GetImageInfoSPNG(ImageIoHandler* imageIO)
{
	TXPErrorCode err = TXP_RETURN_SUCCESS;
	spng_data* img = (spng_data*)imageIO->ILD;

	struct spng_ihdr imageInfo = {};
	int spng_error = spng_get_ihdr(img->ctx, &imageInfo);
	if (spng_error)
	{
		DebugLog(UNITY_LOG_LEVEL_ERROR, "Error - spng failed to get image info: %s\n", spng_strerror(spng_error));
		err = TXP_RETURN_IMG_LIBRARY_FAILED;
		goto exit;
	}
	imageIO->width = imageInfo.width;
	imageIO->height = imageInfo.height;
	switch (imageInfo.color_type)
	{
	case SPNG_COLOR_TYPE_TRUECOLOR:			imageIO->channelCount = 3; break;
	case SPNG_COLOR_TYPE_GRAYSCALE_ALPHA:	imageIO->channelCount = 2; break;
	case SPNG_COLOR_TYPE_TRUECOLOR_ALPHA:	imageIO->channelCount = 4; break;
	case SPNG_COLOR_TYPE_GRAYSCALE:			imageIO->channelCount = 1; break;
	case SPNG_COLOR_TYPE_INDEXED:			
		imageIO->channelCount = 4; 
		img->requiresRemapping = 1;
		break; // if channelcount is 0, 
	default:								
		DebugLog(UNITY_LOG_LEVEL_ERROR, "SLZ TexProc: GetImageInfoSPNG got an unhandled color_type of %d", imageInfo.color_type);
		imageIO->channelCount = 0; 
		break;
	}
	switch (imageInfo.bit_depth)
	{
	case(8): imageIO->channelFormat = CHANNEL_FMT_FIXED8; break;
	case(16): imageIO->channelFormat = CHANNEL_FMT_FIXED16; break;
	default: imageIO->channelFormat = CHANNEL_FMT_UNKNOWN; break;
	}
	// use SPNG to reinterpret indexed images to 8 bit RGBA color
	if (imageInfo.color_type == SPNG_COLOR_TYPE_INDEXED)
	{
		imageIO->channelFormat = CHANNEL_FMT_FIXED8;
	}
	imageIO->yFlip = 1;
exit:
	return err;
}

union tgaOrigin
{
	int32_t intval;
	int16_t origin[2];
};

struct tgaOriginHeader
{
	int64_t dontCare;
	union tgaOrigin origin;
};

static TXPErrorCode GetImageInfoSTB(ImageIoHandler* imageIO)
{
	TXPErrorCode err = TXP_RETURN_SUCCESS;
	stb_data* data = (stb_data*)imageIO->ILD;
	int stb_error = stbi_info(data->utf8path, &imageIO->width, &imageIO->height, &imageIO->channelCount);
	if (stb_error != 1)
	{
		DebugLog(UNITY_LOG_LEVEL_ERROR, "Error - stb library failed to get image info (%d): %s\n", stb_error, stbi__g_failure_reason);
		err = TXP_RETURN_IMG_LIBRARY_FAILED;
		goto exit;
	}
	
	imageIO->yFlip = 1; //Default to assuming the image starts at top left

	//if (imageIO->type == TXP_IMG_TGA)
	//{
	//	FILE* file;
	//	FILE_OPEN(file, imageIO->path.s, PATH_LITERAL("rb"));
	//	struct tgaOriginHeader tgaHeader = {};
	//	fread(&tgaHeader, sizeof(tgaHeader), 1, file);
	//	fclose(file);
	//
	//	if (tgaHeader.origin.origin[0] != 0)
	//	{
	//		
	//		DebugLog(UNITY_LOG_LEVEL_ERROR, "SLZ Tex Proc: TGA file has origin on the right side. This is not supported. Path: %s\n", data->utf8path);
	//		err = TXP_RETURN_IMG_LIBRARY_FAILED;
	//		goto exit;
	//	}
	//
	//	if (tgaHeader.origin.origin[1] != 0)
	//	{
	//		imageIO->yFlip = 1;
	//	}
	//	else
	//	{
	//		imageIO->yFlip = 0;
	//	}
	//
	//
	//}

	if (imageIO->type == TXP_IMG_HDR)
	{
		imageIO->channelFormat = CHANNEL_FMT_FLOAT32;
	}
	else
	{
		int is16bit = stbi_is_16_bit(data->utf8path);
		if (is16bit)
		{
			imageIO->channelFormat = CHANNEL_FMT_FIXED16;
		}
		else
		{
			imageIO->channelFormat = CHANNEL_FMT_FIXED8;
		}
	}

exit:
	return err;
}

static TXPErrorCode GetImageInfoTinyEXR(ImageIoHandler* imageIO)
{
	TXPErrorCode err = TXP_RETURN_SUCCESS;
	texr_data* data = (texr_data*)imageIO->ILD;

	EXRVersion exr_version;
	EXRHeader exr_header;

	InitEXRHeader(&exr_header);
	int exr_error = ParseEXRVersionFromFile(&exr_version, data->utf8path);
	if (exr_error != TINYEXR_SUCCESS) {
		DebugLog(UNITY_LOG_LEVEL_ERROR, "Tiny EXR Data: failed to get version number, error code: %d, path: %s\n", exr_error, data->utf8path);
		err = TXP_RETURN_IMG_LIBRARY_FAILED;
		goto cleanupHeader;
	}

	const char* errorMsg;
	exr_error = ParseEXRHeaderFromFile(&exr_header, &exr_version, data->utf8path, &errorMsg);
	if (exr_error != TINYEXR_SUCCESS) {
		DebugLog(UNITY_LOG_LEVEL_ERROR, "Tiny EXR Data: failed to get version number, error code: %d, path: %s\n%s", exr_error, data->utf8path, errorMsg);
		FreeEXRErrorMessage(errorMsg);
		err = TXP_RETURN_IMG_LIBRARY_FAILED;
		goto cleanupHeader;
	}

	imageIO->channelFormat = CHANNEL_FMT_FLOAT16;
	for (int i = 0; i < exr_header.num_channels; i++) {
		if (exr_header.pixel_types[i] != TINYEXR_PIXELTYPE_HALF) {
			imageIO->channelFormat = CHANNEL_FMT_FLOAT32;
			break;
		}
	}

	// Dont do layers for now, too complex
	
	//int layers = 0;
	//char* errorMsg2;
	//retCode = EXRLayers(data->utf8path, &data->layerNames, &layers, &errorMsg2);
	//if (retCode != TINYEXR_SUCCESS)
	//{
	//	fprintf(stderr, "Tiny EXR Data: failed on getting layers, error: %s\n", errorMsg2);
	//	FreeEXRErrorMessage(errorMsg2);
	//}
	
	//if (layers > 0)
	//{
	//	fprintf(stdout, "EXR Contains %i Layers\n", layers);
	//	for (size_t i = 0; i < layers; ++i) {
	//		fprintf(stdout, "Layer %i : %s\n", i + 1, data->layerNames[i]);
	//	}
	//}


	imageIO->channelCount = 4; //tinyEXR basic load methods always output 4 channels
	imageIO->width = exr_header.data_window.max_x - exr_header.data_window.min_x + 1;
	imageIO->height = exr_header.data_window.max_y - exr_header.data_window.min_y + 1;
	imageIO->depth = 1;
	imageIO->yFlip = 1;

cleanupHeader:
	FreeEXRHeader(&exr_header);
exit:
	return err;
}

TXPErrorCode GetImageInfo(ImageIoHandler* imageIO)
{
	switch (imageIO->imageLibrary)
	{
	case (TXP_IMGLIB_SPNG): return GetImageInfoSPNG(imageIO); break;
	case (TXP_IMGLIB_STB): return GetImageInfoSTB(imageIO); break;
	case (TXP_IMGLIB_EXR): return GetImageInfoTinyEXR(imageIO); break;
	default: 
		DebugLog(UNITY_LOG_LEVEL_ERROR, "SLZ TexProc: GetImageInfo got bad image library enum %d", imageIO->imageLibrary);
		break;
	}
	
	return TXP_RETURN_INVALID_IMG_LIBRARY;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// Read Image
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static TXPErrorCode IOReadImageSPNG(ImageIoHandler* imageIO)
{
	TXPErrorCode err = TXP_RETURN_SUCCESS;
	spng_data* img = (spng_data*)imageIO->ILD;

	size_t bufferSize;
	//int bytes = ChannelFmtToBytes(imageIO->channelFormat);
	int spngFmt = img->requiresRemapping ? SPNG_FMT_RGBA8 : SPNG_FMT_PNG;
	int spng_error = spng_decoded_image_size(img->ctx, spngFmt, &bufferSize);
	if (spng_error)
	{
		DebugLog(UNITY_LOG_LEVEL_ERROR, "SPNG image read: Failed to calculate decoded image size! (%s)\n", spng_strerror(spng_error));
		err = TXP_RETURN_IMG_LIBRARY_FAILED;
		goto cleanup;
	}

	//printf("SPNG image read: Decoded bytes: %llu\nCalculated decoded bytes: %d\n", bufferSize, bytes * imageIO->channelCount * imageIO->width * imageIO->height * imageIO->depth);
	void* imageBuffer = malloc(bufferSize);
	imageIO->imageBuffer = imageBuffer;
	if (imageBuffer == NULL)
	{
		DebugLog(UNITY_LOG_LEVEL_ERROR, "SPNG image read: Failed to allocate memory!\n");
		err = TXP_RETURN_ALLOC_FAILED;
		goto cleanup;
	}
	spng_error = spng_decode_image(img->ctx, imageBuffer, bufferSize, spngFmt, 0);
	if (spng_error)
	{
		DebugLog(UNITY_LOG_LEVEL_ERROR, "SPNG image read: Failed to decode image! (%s)\n", spng_strerror(spng_error));
		err = TXP_RETURN_IMG_LIBRARY_FAILED;
		free(imageBuffer);
		imageIO->imageBuffer = NULL;
		goto cleanup;
	}
cleanup:
	spng_ctx_free(img->ctx);
	img->ctx = NULL;
	fclose(img->file);
	img->file = NULL;
exit:
	return err;
}

static TXPErrorCode IOReadImageSTB(ImageIoHandler* imageIO)
{
	TXPErrorCode err = TXP_RETURN_SUCCESS;

	stb_data* img = (stb_data*)imageIO->ILD;
	int channelCount = imageIO->channelCount;
	if (imageIO->channelFormat == CHANNEL_FMT_FIXED8)
	{
		imageIO->imageBuffer = stbi_load(img->utf8path, &imageIO->width, &imageIO->height, &imageIO->channelCount, channelCount);
	}
	else if (imageIO->channelFormat == CHANNEL_FMT_FIXED16)
	{
		imageIO->imageBuffer = stbi_load_16(img->utf8path, &imageIO->width, &imageIO->height, &imageIO->channelCount, channelCount);
	}
	else if (imageIO->channelFormat == CHANNEL_FMT_FLOAT32 || imageIO->channelFormat == CHANNEL_FMT_FLOAT16)
	{
		imageIO->imageBuffer = stbi_loadf(img->utf8path, &imageIO->width, &imageIO->height, &imageIO->channelCount, channelCount);
	}
	if (imageIO->imageBuffer == NULL)
	{
		DebugLog(UNITY_LOG_LEVEL_ERROR, "STB Image Read: Failed to load image at path: %s\n", img->utf8path);
		err = TXP_RETURN_IMG_LIBRARY_FAILED;
	}
	return err;
}

static TXPErrorCode IOReadImageTEXR(ImageIoHandler* imageIO)
{
	TXPErrorCode err = TXP_RETURN_SUCCESS;

	texr_data* img = (texr_data*)imageIO->ILD;
	const char* errorMsg = NULL;
	int exr_error = 0;
	if (imageIO->channelFormat == CHANNEL_FMT_FLOAT16)
	{
		exr_error = LoadEXRWithLayerF16((unsigned short**)(&imageIO->imageBuffer), &imageIO->width, &imageIO->height, img->utf8path, NULL, &errorMsg);
	}
	else
	{
		exr_error = LoadEXRWithLayer((float**)(&imageIO->imageBuffer), &imageIO->width, &imageIO->height, img->utf8path, NULL, &errorMsg);
	}
	if (exr_error != TINYEXR_SUCCESS)
	{
		DebugLog(UNITY_LOG_LEVEL_ERROR, "TinyEXR failed to read image at path %s\n Tiny EXR Error (%d): %s", img->utf8path, exr_error, errorMsg);
		err = TXP_RETURN_IMG_LIBRARY_FAILED;
	}
	return err;
}

TXPErrorCode IOReadImage(ImageIoHandler* imageIO)
{
	switch (imageIO->imageLibrary)
	{
	case (TXP_IMGLIB_SPNG): return IOReadImageSPNG(imageIO);
	case (TXP_IMGLIB_STB): return IOReadImageSTB(imageIO);
	case (TXP_IMGLIB_EXR): return IOReadImageTEXR(imageIO);
	default: break;
	}
	DebugLog(UNITY_LOG_LEVEL_ERROR, "SLZ TexProc: IOReadImage got bad image library enum %d", imageIO->imageLibrary);
	return TXP_RETURN_INVALID_IMG_LIBRARY;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// Dispose Image
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void DisposeImageBufferSPNG(ImageIoHandler* imageIO)
{
	if (imageIO != NULL && imageIO->imageBuffer != NULL)
	{
		free(imageIO->imageBuffer);
		imageIO->imageBuffer = NULL;
	}
}

static void DisposeImageBufferSTB(ImageIoHandler* imageIO)
{
	if (imageIO != NULL && imageIO->imageBuffer != NULL)
	{
		free(imageIO->imageBuffer);
		imageIO->imageBuffer = NULL;
	}
}

static void DisposeImageBufferTEXR(ImageIoHandler* imageIO)
{
	if (imageIO != NULL && imageIO->imageBuffer != NULL)
	{
		TinyEXRFreeMemory(imageIO->imageBuffer);
		imageIO->imageBuffer = NULL;
	}
}

void DisposeImageBuffer(ImageIoHandler* imageIO)
{
	if (imageIO == NULL)
	{
		return;
	}
	switch (imageIO->imageLibrary)
	{
	case (TXP_IMGLIB_SPNG): DisposeImageBufferSPNG(imageIO); break;
	case (TXP_IMGLIB_STB): DisposeImageBufferSTB(imageIO); break;
	case (TXP_IMGLIB_EXR): DisposeImageBufferTEXR(imageIO); break;
	default: break;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// Get Image IO
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

int InitImageHandler(const PathString* path, ImageIoHandler* handler)
{
	TXPErrorCode err = TXP_RETURN_SUCCESS;
	FILE* file;
	// Try getting the file type from the magic number
	FILE_OPEN(file, path->s, PATH_LITERAL("rb"));
	if (file == NULL)
	{
		char* pathStr = GetPlatShortPath(path->s);
		DebugLog(UNITY_LOG_LEVEL_ERROR, "Failed to open file at %s\n", pathStr);
		FreePlatShortPath(pathStr);
	}
	handler->type = GetFileTypeFromMagicNumber(file);
	fclose(file);
	// Otherwise, fall back to using the file extension
	if (handler->type == TXP_IMG_INVALID)
	{
		char* pathStr = GetPlatShortPath(path->s);
		DebugLog(UNITY_LOG_LEVEL_WARN, "SLZ Tex Proc: Failed to get file type from magic number, falling back to file extension. File Path: %s\n", pathStr);
		FreePlatShortPath(pathStr);
		handler->type = GetFileTypeFromExt(path);
	}
	if (handler->type == TXP_IMG_INVALID) { err = TXP_RETURN_INVALID_TEX_FORMAT; goto cleanup; }

	handler->imageLibrary = GetLibraryForType(handler->type);
	if (handler->imageLibrary == TXP_IMGLIB_NONE) { err = TXP_RETURN_NO_IMG_LIBRARY; goto cleanup; }

	size_t pathByteLen = sizeof(PATH_CHAR) * (path->length + 1);
	handler->path.s = (PATH_CHAR*)malloc(pathByteLen);
	if (handler->path.s == NULL) { err = TXP_RETURN_ALLOC_FAILED; goto exit; }
	memcpy(handler->path.s, path->s, pathByteLen);
	handler->path.length = path->length;

	err = GetLibraryFileData(&(handler->ILD), handler->imageLibrary, &handler->path);

cleanup:
	if (err != TXP_RETURN_SUCCESS)
	{
		handler->type = TXP_IMG_INVALID;
	}
	
exit:
	return (int)err;
}

void DisposeImageHandler(ImageIoHandler* handler)
{
	
	if (handler == NULL || handler->type == TXP_IMG_INVALID)
	{
		return;
	}
	DisposeLibraryFileData(handler);
	if (handler->path.s != NULL)
	{
		free(handler->path.s);
		handler->path.s = NULL;
		handler->path.length = 0;
	}
	DisposeImageBuffer(handler);
}


void DebugTestSpng(TxpTex2D* outTex)
{
	spng_ctx* enc = spng_ctx_new(SPNG_CTX_ENCODER);
	spng_set_option(enc, SPNG_ENCODE_TO_BUFFER, 1);
	
	struct spng_ihdr ihdr2 =
	{
		.width = outTex->resolution[0].x,
		.height = outTex->resolution[0].y,
		.bit_depth = 16,
		.color_type = SPNG_COLOR_TYPE_TRUECOLOR_ALPHA
	};

	switch (outTex->format)
	{
	case (FMT_RGB8) : 
		ihdr2.color_type = SPNG_COLOR_TYPE_TRUECOLOR;
		ihdr2.bit_depth = 8;
		break;
	case (FMT_RGBA8):
		ihdr2.color_type = SPNG_COLOR_TYPE_TRUECOLOR_ALPHA;
		ihdr2.bit_depth = 8;
		break;
	case (FMT_RGB16):
		ihdr2.color_type = SPNG_COLOR_TYPE_TRUECOLOR;
		ihdr2.bit_depth = 16;
		break;
	case (FMT_RGBA16):
		ihdr2.color_type = SPNG_COLOR_TYPE_TRUECOLOR_ALPHA;
		ihdr2.bit_depth = 16;
		break;
	default: DebugLog(UNITY_LOG_LEVEL_ERROR, "SPNG writer: invalid input texture format. SPNG can only output 8 or 16 bit RGB or RGBA images"); break;
	}

	PATH_CHAR outName[] = PATH_LITERAL("outImgX.png");
	int outNameLen = sizeof(outName) / sizeof(PATH_CHAR);
	PATH_CHAR outPath[g_libraryPath->length + outNameLen];
	memcpy(outPath, g_libraryPath->s, g_libraryPath->length * sizeof(PATH_CHAR));
	memcpy(&outPath[g_libraryPath->length], outName, sizeof(outName));
	int mipCharIdx = g_libraryPath->length + outNameLen - 6;
	PATH_CHAR mipChar[4];
	for (int mIdx = 0; mIdx < outTex->mipCount; mIdx++)
	{
		PATH_SPRINT(mipChar, PATH_LITERAL("%x"), mIdx);
		outPath[mipCharIdx] = mipChar[0];
		//PATH_PRINT(outPath);
		//printf("\n");
		ihdr2.width = outTex->resolution[mIdx].x;
		ihdr2.height = outTex->resolution[mIdx].y;
		spng_ctx* enc = spng_ctx_new(SPNG_CTX_ENCODER);
		spng_set_option(enc, SPNG_ENCODE_TO_BUFFER, 1);
		spng_set_ihdr(enc, &ihdr2);
		spng_encode_image(enc, outTex->mips[mIdx], outTex->resolution[mIdx].x * outTex->resolution[mIdx].y * TextureFormatSize(outTex->format), SPNG_FMT_PNG, SPNG_ENCODE_FINALIZE);
		size_t outSize;
		int error;
		void* png = spng_get_png_buffer(enc, &outSize, &error);
		DebugLog(UNITY_LOG_LEVEL_LOG, "SPNG Error Code: %d\n", error);


		FILE* outPng;
		FILE_OPEN(outPng, outPath, PATH_LITERAL("wb"));
		fwrite(png, 1, outSize, outPng);
		fclose(outPng);
		free(png);
		spng_ctx_free(enc);
	}
}