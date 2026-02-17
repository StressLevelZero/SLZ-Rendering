#ifndef TINYEXR_H_FP16PATCH
#define TINYEXR_H_FP16PATCH

#include "tinyexr.h"

#ifdef __cplusplus
extern "C" {
#endif
    extern int LoadEXRWithLayerF16(unsigned short** out_rgba, int* width, int* height,
        const char* filename, const char* layername,
        const char** err);
    // Use this to free any memory allocated by TinyEXR, its always better for a library to be in charge of freeing the memory it allocates
    extern void TinyEXRFreeMemory(void* memory);
#ifdef __cplusplus
}
#endif

#endif