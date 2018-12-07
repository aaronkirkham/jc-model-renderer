#pragma once

#include "../../graphics/types.h"
#include <cstdint>

namespace oo2
{
enum OodleLZCompressionLevel {
    OodleLZCompressionLevel_HyperFast4 = 0,
    OodleLZCompressionLevel_HyperFast3,
    OodleLZCompressionLevel_HyperFast2,
    OodleLZCompressionLevel_HyperFast1,
    OodleLZCompressionLevel_None,
    OodleLZCompressionLevel_SuperFast,
    OodleLZCompressionLevel_VeryFast,
    OodleLZCompressionLevel_Fast,
    OodleLZCompressionLevel_Normal,
    OodleLZCompressionLevel_Optimal1,
    OodleLZCompressionLevel_Optimal2,
    OodleLZCompressionLevel_Optimal3,
    OodleLZCompressionLevel_Optimal4,
    OodleLZCompressionLevel_Optimal5,
    OodleLZCompressionLevel_TooHigh,
};

enum OodleLZCompresor {
    OodleLZCompresor_LZH = 0,
    OodleLZCompresor_LZHLW,
    OodleLZCompresor_LZNIB,
    OodleLZCompresor_None,
    OodleLZCompresor_LZB16,
    OodleLZCompresor_LZBLW,
    OodleLZCompresor_LZA,
    OodleLZCompresor_LZNA,
    OodleLZCompresor_Kraken,
    OodleLZCompresor_Mermaid,
    OodleLZCompresor_BitKnit,
    OodleLZCompresor_Selkie,
    OodleLZCompresor_Hydra,
    OodleLZCompresor_Leviathan,
};

using OodleLZ_Compress_t   = int64_t (*)(OodleLZCompresor, const void*, int64_t, const void*, OodleLZCompressionLevel,
                                       uint32_t*, int64_t, int64_t, int64_t, int64_t);
using OodleLZ_Decompress_t = int64_t (*)(const void*, int64_t, const void*, int64_t, int32_t, int64_t, int64_t, int64_t,
                                         int64_t, void*, void*, int64_t, int64_t, int64_t);

static OodleLZ_Compress_t   OodleLZ_Compress_original   = nullptr;
static OodleLZ_Decompress_t OodleLZ_Decompress_original = nullptr;

static int64_t OodleLZ_GetCompressedBufferSizeNeeded(int64_t size)
{
    return (size + 274 * ((size + 0x3FFFF) / 0x40000));
}

static int64_t OodleLZ_Compress(const void* buffer, int64_t buffer_size, const void* output)
{
    return OodleLZ_Compress_original(OodleLZCompresor_Kraken, buffer, buffer_size, output, OodleLZCompressionLevel_None,
                                     0, 0, 0, 0, 0);
}

static int64_t OodleLZ_Compress(const FileBuffer* buffer, FileBuffer* output)
{
    const auto bound = OodleLZ_GetCompressedBufferSizeNeeded(buffer->size());
    output->resize(bound);

    const auto size = OodleLZ_Compress(buffer->data(), buffer->size(), output->data());
	output->resize(size);
	return size;
}

static int64_t OodleLZ_Decompress(const void* buffer, int64_t buffer_size, const void* output, int64_t output_size)
{
    return OodleLZ_Decompress_original(buffer, buffer_size, output, output_size, 1, 0, 0, 0, 0, nullptr, nullptr, 0, 0,
                                       3);
}

static int64_t OodleLZ_Decompress(const FileBuffer* buffer, FileBuffer* output)
{
    return OodleLZ_Decompress(buffer->data(), buffer->size(), output->data(), output->size());
}
}; // namespace oo2
