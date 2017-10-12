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
    min16int4 uvs : TEXCOORD0;
    float normal : NORMAL0;
    float tangent : TANGENT0;
    float colour : COLOR0;
};

struct VertexOut
{
	float4 position : SV_POSITION;
    float2 tex : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float4 colour : COLOR;
};

VertexOut main(VertexIn input)
{
    // unpack the vertex position
    float3 unpacked = float3(unpack_s16(input.position.x), unpack_s16(input.position.y), unpack_s16(input.position.z)) * scale;

    VertexOut output;
    output.position = mul(worldMatrix, float4(unpacked, 1));
    output.position = mul(viewProjection, output.position);
    output.tex = float2(unpack_s16(input.uvs.x) * uvExtent.x, unpack_s16(input.uvs.y) * uvExtent.y);
    output.normal = mul(unpack_vec3(input.normal, false), worldMatrix);
    output.tangent = mul(unpack_vec3(input.tangent, false), worldMatrix);
    output.colour = colour;

    return output;
}