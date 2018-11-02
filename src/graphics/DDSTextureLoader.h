//--------------------------------------------------------------------------------------
// File: DDSTextureLoader.h
//
// Functions for loading a DDS texture and creating a Direct3D runtime resource for it
//
// Note these functions are useful as a light-weight runtime loader for DDS files. For
// a full-featured DDS file reader, writer, and texture processing pipeline see
// the 'Texconv' sample and the 'DirectXTex' library.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// http://go.microsoft.com/fwlink/?LinkId=248926
// http://go.microsoft.com/fwlink/?LinkId=248929
//--------------------------------------------------------------------------------------

#pragma once

#include <d3d11_1.h>
#include <stdint.h>

#pragma pack(push, 1)

const uint32_t DDS_MAGIC = 0x20534444; // "DDS "

struct DDS_PIXELFORMAT {
    uint32_t size;
    uint32_t flags;
    uint32_t fourCC;
    uint32_t RGBBitCount;
    uint32_t RBitMask;
    uint32_t GBitMask;
    uint32_t BBitMask;
    uint32_t ABitMask;
};

#define DDPF_ALPHAPIXELS 0x1
#define DDPF_ALPHA 0x2
#define DDPF_FOURCC 0x4
#define DDPF_RGB 0x40
#define DDPF_YUV 0x200
#define DDPF_LUMINANCE 0x20000

#define DDSD_CAPS 0x1
#define DDSD_HEIGHT 0x2
#define DDSD_WIDTH 0x4
#define DDSD_PITCH 0x8
#define DDSD_PIXELFORMAT 0x1000
#define DDSD_MIPMAPCOUNT 0x20000
#define DDSD_LINEARSIZE 0x80000
#define DDSD_DEPTH 0x800000

#define DDS_TEXTURE (DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT)

#define DDSCAPS_COMPLEX 0x8
#define DDSCAPS_MIPMAP 0x400000
#define DDSCAPS_TEXTURE 0x1000

#define DDS_FOURCC DDPF_FOURCC
#define DDS_RGB DDPF_RGB
#define DDS_LUMINANCE DDPF_LUMINANCE
#define DDS_ALPHA DDPF_ALPHA
#define DDS_BUMPDUDV 0x00080000 // DDPF_BUMPDUDV

#define DDS_HEADER_FLAGS_VOLUME DDSD_DEPTH

#define DDS_HEIGHT DDSD_HEIGHT
#define DDS_WIDTH DDSD_WIDTH

#define DDS_CUBEMAP_POSITIVEX 0x00000600 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEX
#define DDS_CUBEMAP_NEGATIVEX 0x00000a00 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEX
#define DDS_CUBEMAP_POSITIVEY 0x00001200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEY
#define DDS_CUBEMAP_NEGATIVEY 0x00002200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEY
#define DDS_CUBEMAP_POSITIVEZ 0x00004200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEZ
#define DDS_CUBEMAP_NEGATIVEZ 0x00008200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEZ

#define DDS_CUBEMAP_ALLFACES                                                                                           \
    (DDS_CUBEMAP_POSITIVEX | DDS_CUBEMAP_NEGATIVEX | DDS_CUBEMAP_POSITIVEY | DDS_CUBEMAP_NEGATIVEY                     \
     | DDS_CUBEMAP_POSITIVEZ | DDS_CUBEMAP_NEGATIVEZ)

#define DDS_CUBEMAP 0x00000200 // DDSCAPS2_CUBEMAP

enum DDS_MISC_FLAGS2 {
    DDS_MISC_FLAGS2_ALPHA_MODE_MASK = 0x7L,
};

struct DDS_HEADER {
    uint32_t        size;
    uint32_t        flags;
    uint32_t        height;
    uint32_t        width;
    uint32_t        pitchOrLinearSize;
    uint32_t        depth; // only if DDS_HEADER_FLAGS_VOLUME is set in flags
    uint32_t        mipMapCount;
    uint32_t        reserved1[11];
    DDS_PIXELFORMAT ddspf;
    uint32_t        caps;
    uint32_t        caps2;
    uint32_t        caps3;
    uint32_t        caps4;
    uint32_t        reserved2;
};

struct DDS_HEADER_DXT10 {
    DXGI_FORMAT dxgiFormat;
    uint32_t    resourceDimension;
    uint32_t    miscFlag; // see D3D11_RESOURCE_MISC_FLAG
    uint32_t    arraySize;
    uint32_t    miscFlags2;
};

#pragma pack(pop)

namespace DirectX
{
enum DDS_ALPHA_MODE {
    DDS_ALPHA_MODE_UNKNOWN       = 0,
    DDS_ALPHA_MODE_STRAIGHT      = 1,
    DDS_ALPHA_MODE_PREMULTIPLIED = 2,
    DDS_ALPHA_MODE_OPAQUE        = 3,
    DDS_ALPHA_MODE_CUSTOM        = 4,
};

// Standard version
HRESULT CreateDDSTextureFromMemory(_In_ ID3D11Device* d3dDevice, _In_reads_bytes_(ddsDataSize) const uint8_t* ddsData,
                                   _In_ size_t ddsDataSize, _Outptr_opt_ ID3D11Resource** texture,
                                   _Outptr_opt_ ID3D11ShaderResourceView** textureView, _In_ size_t maxsize = 0,
                                   _Out_opt_ DDS_ALPHA_MODE* alphaMode = nullptr);

HRESULT CreateDDSTextureFromFile(_In_ ID3D11Device* d3dDevice, _In_z_ const wchar_t* szFileName,
                                 _Outptr_opt_ ID3D11Resource** texture,
                                 _Outptr_opt_ ID3D11ShaderResourceView** textureView, _In_ size_t maxsize = 0,
                                 _Out_opt_ DDS_ALPHA_MODE* alphaMode = nullptr);

// Standard version with optional auto-gen mipmap support
HRESULT CreateDDSTextureFromMemory(_In_ ID3D11Device* d3dDevice, _In_opt_ ID3D11DeviceContext* d3dContext,
                                   _In_reads_bytes_(ddsDataSize) const uint8_t* ddsData, _In_ size_t ddsDataSize,
                                   _Outptr_opt_ ID3D11Resource** texture,
                                   _Outptr_opt_ ID3D11ShaderResourceView** textureView, _In_ size_t maxsize = 0,
                                   _Out_opt_ DDS_ALPHA_MODE* alphaMode = nullptr);

HRESULT CreateDDSTextureFromFile(_In_ ID3D11Device* d3dDevice, _In_opt_ ID3D11DeviceContext* d3dContext,
                                 _In_z_ const wchar_t* szFileName, _Outptr_opt_ ID3D11Resource** texture,
                                 _Outptr_opt_ ID3D11ShaderResourceView** textureView, _In_ size_t maxsize = 0,
                                 _Out_opt_ DDS_ALPHA_MODE* alphaMode = nullptr);

// Extended version
HRESULT CreateDDSTextureFromMemoryEx(_In_ ID3D11Device* d3dDevice, _In_reads_bytes_(ddsDataSize) const uint8_t* ddsData,
                                     _In_ size_t ddsDataSize, _In_ size_t maxsize, _In_ D3D11_USAGE usage,
                                     _In_ unsigned int bindFlags, _In_ unsigned int cpuAccessFlags,
                                     _In_ unsigned int miscFlags, _In_ bool forceSRGB,
                                     _Outptr_opt_ ID3D11Resource** texture,
                                     _Outptr_opt_ ID3D11ShaderResourceView** textureView,
                                     _Out_opt_ DDS_ALPHA_MODE* alphaMode = nullptr);

HRESULT CreateDDSTextureFromFileEx(_In_ ID3D11Device* d3dDevice, _In_z_ const wchar_t* szFileName, _In_ size_t maxsize,
                                   _In_ D3D11_USAGE usage, _In_ unsigned int bindFlags,
                                   _In_ unsigned int cpuAccessFlags, _In_ unsigned int miscFlags, _In_ bool forceSRGB,
                                   _Outptr_opt_ ID3D11Resource** texture,
                                   _Outptr_opt_ ID3D11ShaderResourceView** textureView,
                                   _Out_opt_ DDS_ALPHA_MODE* alphaMode = nullptr);

// Extended version with optional auto-gen mipmap support
HRESULT CreateDDSTextureFromMemoryEx(_In_ ID3D11Device* d3dDevice, _In_opt_ ID3D11DeviceContext* d3dContext,
                                     _In_reads_bytes_(ddsDataSize) const uint8_t* ddsData, _In_ size_t ddsDataSize,
                                     _In_ size_t maxsize, _In_ D3D11_USAGE usage, _In_ unsigned int bindFlags,
                                     _In_ unsigned int cpuAccessFlags, _In_ unsigned int miscFlags, _In_ bool forceSRGB,
                                     _Outptr_opt_ ID3D11Resource** texture,
                                     _Outptr_opt_ ID3D11ShaderResourceView** textureView,
                                     _Out_opt_ DDS_ALPHA_MODE* alphaMode = nullptr);

HRESULT CreateDDSTextureFromFileEx(_In_ ID3D11Device* d3dDevice, _In_opt_ ID3D11DeviceContext* d3dContext,
                                   _In_z_ const wchar_t* szFileName, _In_ size_t maxsize, _In_ D3D11_USAGE usage,
                                   _In_ unsigned int bindFlags, _In_ unsigned int cpuAccessFlags,
                                   _In_ unsigned int miscFlags, _In_ bool forceSRGB,
                                   _Outptr_opt_ ID3D11Resource** texture,
                                   _Outptr_opt_ ID3D11ShaderResourceView** textureView,
                                   _Out_opt_ DDS_ALPHA_MODE* alphaMode = nullptr);
} // namespace DirectX
