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
    float3 position : POSITION0;
    float2 uv0 : TEXCOORD0;
    float2 uv1 : TEXCOORD1;
    float normal : NORMAL0;
    float tangent : TANGENT0;
    float2 uv2 : TEXCOORD2;
};

struct VertexOut
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float2 tex2 : TEXCOORD1;
    float2 tex3 : TEXCOORD2;
};

VertexOut main(VertexIn input)
{
    VertexOut output;
    output.position = mul(worldMatrix, float4(input.position, 1));
    output.position = mul(viewProjection, output.position);
    output.tex = input.uv0;
    output.tex2 = input.uv1;
    output.tex3 = input.uv2;

    return output;
}