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
};

struct VertexOut
{
    float4 position : SV_POSITION;
};

VertexOut main(VertexIn input)
{
    // unpack the vertex position
    float3 unpacked = float3(unpack_s16(input.position.x), unpack_s16(input.position.y), unpack_s16(input.position.z));

    VertexOut output;
    output.position = mul(worldMatrix, float4(unpacked, 1));
    output.position = mul(viewProjection, output.position);

    return output;
}