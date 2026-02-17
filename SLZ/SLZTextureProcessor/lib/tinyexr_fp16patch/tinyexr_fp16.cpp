#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif

#define TINYEXR_IMPLEMENTATION
#include "tinyexr_fp16.h"


void TinyEXRFreeMemory(void* memory)
{
    free(memory);
}

// Literally just a duplicate of LoadEXRWithLayer but 1) set the requested pixel type to half instead of float, 2) allocate all memory as unsigned short and return unsigned short data.
// Copied at commit 6c8742cc8145c8f629698cd8248900990946d6b1
// If tinyEXR updates LoadEXRWithLayer, it'll probably be necessary to copy-paste the function again, change it to request half precision, and change floats to unsigned shorts
int LoadEXRWithLayerF16(unsigned short** out_rgba, int* width, int* height,
    const char* filename, const char* layername,
    const char** err) {
    if (out_rgba == NULL) {
        tinyexr::SetErrorMessage("Invalid argument for LoadEXR()", err);
        return TINYEXR_ERROR_INVALID_ARGUMENT;
    }

    EXRVersion exr_version;
    EXRImage exr_image;
    EXRHeader exr_header;
    InitEXRHeader(&exr_header);
    InitEXRImage(&exr_image);

    {
        int ret = ParseEXRVersionFromFile(&exr_version, filename);
        if (ret != TINYEXR_SUCCESS) {
            std::stringstream ss;
            ss << "Failed to open EXR file or read version info from EXR file. code("
                << ret << ")";
            tinyexr::SetErrorMessage(ss.str(), err);
            return ret;
        }

        if (exr_version.multipart || exr_version.non_image) {
            tinyexr::SetErrorMessage(
                "Loading multipart or DeepImage is not supported  in LoadEXR() API",
                err);
            return TINYEXR_ERROR_INVALID_DATA;  // @fixme.
        }
    }

    {
        int ret = ParseEXRHeaderFromFile(&exr_header, &exr_version, filename, err);
        if (ret != TINYEXR_SUCCESS) {
            FreeEXRHeader(&exr_header);
            return ret;
        }
    }

    // Read FLOAT channel as HALF.
    for (int i = 0; i < exr_header.num_channels; i++) {
        if (exr_header.pixel_types[i] != TINYEXR_PIXELTYPE_HALF) {
            exr_header.requested_pixel_types[i] = TINYEXR_PIXELTYPE_HALF;
        }
    }

    // TODO: Probably limit loading to layers (channels) selected by layer index
    {
        int ret = LoadEXRImageFromFile(&exr_image, &exr_header, filename, err);
        if (ret != TINYEXR_SUCCESS) {
            FreeEXRHeader(&exr_header);
            return ret;
        }
    }

    // RGBA
    int idxR = -1;
    int idxG = -1;
    int idxB = -1;
    int idxA = -1;

    std::vector<std::string> layer_names;
    tinyexr::GetLayers(exr_header, layer_names);

    std::vector<tinyexr::LayerChannel> channels;
    tinyexr::ChannelsInLayer(
        exr_header, layername == NULL ? "" : std::string(layername), channels);

    if (channels.size() < 1) {
        if (layername == NULL) {
            tinyexr::SetErrorMessage(
                "Layer Not Found. Seems EXR contains channels with layer(e.g. "
                "`diffuse.R`). if you are using LoadEXR(), please try "
                "LoadEXRWithLayer(). LoadEXR() cannot load EXR having channels with "
                "layer.",
                err);

        }
        else {
            tinyexr::SetErrorMessage("Layer Not Found", err);
        }
        FreeEXRHeader(&exr_header);
        FreeEXRImage(&exr_image);
        return TINYEXR_ERROR_LAYER_NOT_FOUND;
    }

    size_t ch_count = channels.size() < 4 ? channels.size() : 4;
    for (size_t c = 0; c < ch_count; c++) {
        const tinyexr::LayerChannel& ch = channels[c];

        if (ch.name == "R") {
            idxR = int(ch.index);
        }
        else if (ch.name == "G") {
            idxG = int(ch.index);
        }
        else if (ch.name == "B") {
            idxB = int(ch.index);
        }
        else if (ch.name == "A") {
            idxA = int(ch.index);
        }
    }

    if (channels.size() == 1) {
        int chIdx = int(channels.front().index);
        // Grayscale channel only.

        (*out_rgba) = reinterpret_cast<unsigned short*>(malloc(
            4 * sizeof(unsigned short) * static_cast<size_t>(exr_image.width) *
            static_cast<size_t>(exr_image.height)));

        if (exr_header.tiled) {
            const size_t tile_size_x = static_cast<size_t>(exr_header.tile_size_x);
            const size_t tile_size_y = static_cast<size_t>(exr_header.tile_size_y);
            for (int it = 0; it < exr_image.num_tiles; it++) {
                for (size_t j = 0; j < tile_size_y; j++) {
                    for (size_t i = 0; i < tile_size_x; i++) {
                        const size_t ii =
                            static_cast<size_t>(exr_image.tiles[it].offset_x) *
                            tile_size_x +
                            i;
                        const size_t jj =
                            static_cast<size_t>(exr_image.tiles[it].offset_y) *
                            tile_size_y +
                            j;
                        const size_t idx = ii + jj * static_cast<size_t>(exr_image.width);

                        // out of region check.
                        if (ii >= static_cast<size_t>(exr_image.width)) {
                            continue;
                        }
                        if (jj >= static_cast<size_t>(exr_image.height)) {
                            continue;
                        }
                        const size_t srcIdx = i + j * tile_size_x;
                        unsigned char** src = exr_image.tiles[it].images;
                        (*out_rgba)[4 * idx + 0] =
                            reinterpret_cast<unsigned short**>(src)[chIdx][srcIdx];
                        (*out_rgba)[4 * idx + 1] =
                            reinterpret_cast<unsigned short**>(src)[chIdx][srcIdx];
                        (*out_rgba)[4 * idx + 2] =
                            reinterpret_cast<unsigned short**>(src)[chIdx][srcIdx];
                        (*out_rgba)[4 * idx + 3] =
                            reinterpret_cast<unsigned short**>(src)[chIdx][srcIdx];
                    }
                }
            }
        }
        else {
            const size_t pixel_size = static_cast<size_t>(exr_image.width) *
                static_cast<size_t>(exr_image.height);
            for (size_t i = 0; i < pixel_size; i++) {
                const unsigned short val =
                    reinterpret_cast<unsigned short**>(exr_image.images)[chIdx][i];
                (*out_rgba)[4 * i + 0] = val;
                (*out_rgba)[4 * i + 1] = val;
                (*out_rgba)[4 * i + 2] = val;
                (*out_rgba)[4 * i + 3] = val;
            }
        }
    }
    else {
        // Assume RGB(A)

        if (idxR == -1) {
            tinyexr::SetErrorMessage("R channel not found", err);

            FreeEXRHeader(&exr_header);
            FreeEXRImage(&exr_image);
            return TINYEXR_ERROR_INVALID_DATA;
        }

        if (idxG == -1) {
            tinyexr::SetErrorMessage("G channel not found", err);
            FreeEXRHeader(&exr_header);
            FreeEXRImage(&exr_image);
            return TINYEXR_ERROR_INVALID_DATA;
        }

        if (idxB == -1) {
            tinyexr::SetErrorMessage("B channel not found", err);
            FreeEXRHeader(&exr_header);
            FreeEXRImage(&exr_image);
            return TINYEXR_ERROR_INVALID_DATA;
        }

        (*out_rgba) = reinterpret_cast<unsigned short*>(malloc(
            4 * sizeof(unsigned short) * static_cast<size_t>(exr_image.width) *
            static_cast<size_t>(exr_image.height)));
        if (exr_header.tiled) {
            const size_t tile_size_x = static_cast<size_t>(exr_header.tile_size_x);
            const size_t tile_size_y = static_cast<size_t>(exr_header.tile_size_y);
            for (int it = 0; it < exr_image.num_tiles; it++) {
                for (size_t j = 0; j < tile_size_y; j++) {
                    for (size_t i = 0; i < tile_size_x; i++) {
                        const size_t ii =
                            static_cast<size_t>(exr_image.tiles[it].offset_x) *
                            tile_size_x +
                            i;
                        const size_t jj =
                            static_cast<size_t>(exr_image.tiles[it].offset_y) *
                            tile_size_y +
                            j;
                        const size_t idx = ii + jj * static_cast<size_t>(exr_image.width);

                        // out of region check.
                        if (ii >= static_cast<size_t>(exr_image.width)) {
                            continue;
                        }
                        if (jj >= static_cast<size_t>(exr_image.height)) {
                            continue;
                        }
                        const size_t srcIdx = i + j * tile_size_x;
                        unsigned char** src = exr_image.tiles[it].images;
                        (*out_rgba)[4 * idx + 0] =
                            reinterpret_cast<unsigned short**>(src)[idxR][srcIdx];
                        (*out_rgba)[4 * idx + 1] =
                            reinterpret_cast<unsigned short**>(src)[idxG][srcIdx];
                        (*out_rgba)[4 * idx + 2] =
                            reinterpret_cast<unsigned short**>(src)[idxB][srcIdx];
                        if (idxA != -1) {
                            (*out_rgba)[4 * idx + 3] =
                                reinterpret_cast<unsigned short**>(src)[idxA][srcIdx];
                        }
                        else {
                            (*out_rgba)[4 * idx + 3] = 1.0;
                        }
                    }
                }
            }
        }
        else {
            const size_t pixel_size = static_cast<size_t>(exr_image.width) *
                static_cast<size_t>(exr_image.height);
            for (size_t i = 0; i < pixel_size; i++) {
                (*out_rgba)[4 * i + 0] =
                    reinterpret_cast<unsigned short**>(exr_image.images)[idxR][i];
                (*out_rgba)[4 * i + 1] =
                    reinterpret_cast<unsigned short**>(exr_image.images)[idxG][i];
                (*out_rgba)[4 * i + 2] =
                    reinterpret_cast<unsigned short**>(exr_image.images)[idxB][i];
                if (idxA != -1) {
                    (*out_rgba)[4 * i + 3] =
                        reinterpret_cast<unsigned short**>(exr_image.images)[idxA][i];
                }
                else {
                    (*out_rgba)[4 * i + 3] = 1.0;
                }
            }
        }
    }

    (*width) = exr_image.width;
    (*height) = exr_image.height;

    FreeEXRHeader(&exr_header);
    FreeEXRImage(&exr_image);

    return TINYEXR_SUCCESS;
}