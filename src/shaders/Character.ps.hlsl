cbuffer MeshConstant : register(b1)
{
    float4x4 worldMatrix;
    float4x4 worldViewInverseTranspose;
    float4 colour;
}

struct PixelIn
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD;
};

Texture2D Diffuse : register(t0);
Texture2D Normal : register(t1);

SamplerState Sampler : register(s0);

float4 main(PixelIn input) : SV_TARGET
{
    return Diffuse.Sample(Sampler, input.tex) * colour;
}