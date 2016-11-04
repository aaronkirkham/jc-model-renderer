#ifndef VERTEXTYPES_H
#define VERTEXTYPES_H

#include <cstdint>
#include <assert.h>
#include <QVector2D>

#pragma pack(push, 1)
struct SPackedAttribute
{
    int32_t format;
    float scale;
    QVector2D uv0Extent;
    QVector2D uv1Extent;
    float colourExtent;
    uint8_t colourR;
    uint8_t colourG;
    uint8_t colourB;
    uint8_t colourA;
};

namespace Vertex
{
    struct Packed
    {
        int16_t x;
        int16_t y;
        int16_t z;
        int16_t pad;
    };

    struct GeneralShortPacked
    {
        int16_t u0;
        int16_t v0;
        int16_t u1;
        int16_t v1;
        float n;
        float t;
        float col;
    };

    struct Tex2UVNormal
    {
        float u0;
        float v0;
        float u1;
        float v1;
        int8_t nx;
        int8_t ny;
        int8_t nz;
        int8_t nw;
    };

    struct PackedCharacterPos4
    {
        int16_t x;
        int16_t y;
        int16_t z;
        int16_t dummy;
        uint8_t w0[4];
        uint8_t i0[4];
        uint32_t tangent_space;
        uint16_t u0;
        uint16_t v0;
    };

    struct Unpacked
    {
        float x;
        float y;
        float z;
        float u0;
        float v0;
        float u1;
        float v1;
        float n;
        float t;
        float col;
    };
    
    struct UnpackedVertexPosition
    {
        float x;
        float y;
        float z;
    };

    struct VertexDeformPos
    {
        float x;
        float y;
        float z;
        float d;
        __int16 wi0;
        __int16 wi1;
        __int16 wi2;
        __int16 wi3;
    };

    struct VertexDeformNormal2
    {
        float u;
        float v;
        float u2;
        float v2;
        float n;
        float t;
        float dn;
        float dt;
    };

    struct VertexNormals
    {
        float u;
        float v;
        float u2;
        float v2;
        float n;
        float t;
    };

    struct UnpackedVertexWithNormal1
    {
        float x;
        float y;
        float z;
        float normal;
    };

    struct PackedTex2UV
    {
        int16_t u0;
        int16_t v0;
        int16_t u1;
        int16_t v1;
    };

    static float expand(int16_t value)
    {
        if (value == -1)
            return -1.0f;

        return (value * (1.0f / 32767));
    }

    static_assert(sizeof(Packed) == 0x8, "Packed has the wrong size!");
    static_assert(sizeof(GeneralShortPacked) == 0x14, "GeneralShortPacked has the wrong size!");
    static_assert(sizeof(Tex2UVNormal) == 0x14, "Tex2UVNormal has the wrong size!");
    static_assert(sizeof(PackedCharacterPos4) == 0x18, "PackedCharacterPos4 has the wrong size!");
    static_assert(sizeof(Unpacked) == 0x28, "Unpacked has the wrong size!");
    static_assert(sizeof(PackedTex2UV) == 0x8, "PackedTex2UV has the wrong size!");
    static_assert(sizeof(UnpackedVertexPosition) == 0xC, "UnpackedVertexPosition has the wrong size!");
    static_assert(sizeof(VertexNormals) == 0x18, "VertexNormals has the wrong size!");
};
#pragma pack(pop)

#endif VERTEXTYPES_H
