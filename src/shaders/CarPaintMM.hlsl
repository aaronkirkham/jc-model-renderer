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
};

struct VertexOut
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD;
};

// @gyp_compile(vs_5_0, vs_main)
VertexOut vs_main(VertexIn input)
{
    VertexOut output;
    output.position = mul(worldMatrix, float4(input.position, 1));
    output.position = mul(viewProjection, output.position);
    output.tex = input.uv0;

    return output;
}

// @gyp_compile(ps_5_0, ps_main)
Texture2D Diffuse : register(t0);
SamplerState Sampler : register(s0);

float4 ps_main(VertexOut input) : SV_TARGET
{
    return Diffuse.Sample(Sampler, input.tex);
}