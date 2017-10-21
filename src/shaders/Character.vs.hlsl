#include "helper.hlsl"

cbuffer FrameConstant : register(b0)
{
    float4x4 viewProjection;
}

cbuffer MeshConstant : register(b1)
{
    float4x4 worldMatrix;
    float4x4 worldViewInverseTranspose;
    float4 colour;
}

cbuffer ModelConstants : register(b2)
{
    float scale;
    float2 uvExtent;
}

struct VertexIn
{
    min16int4 position : POSITION0;
    min16int2 uv : TEXCOORD0;
};

struct VertexOut
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD;
};

VertexOut main(VertexIn input)
{
    // unpack the vertex position
    float3 unpacked = float3(unpack_s16(input.position.x), unpack_s16(input.position.y), unpack_s16(input.position.z)) * scale;

    VertexOut output;
    output.position = mul(worldMatrix, float4(unpacked, 1));
    output.position = mul(viewProjection, output.position);
    output.tex = float2(unpack_s16(input.uv.x), unpack_s16(input.uv.y));

    return output;
}