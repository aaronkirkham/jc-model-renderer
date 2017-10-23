cbuffer MeshConstant : register(b1)
{
    float4x4 worldMatrix;
    float4x4 worldViewInverseTranspose;
    float4 colour;
}

cbuffer ModelConstants : register(b2)
{
    bool hasLayeredUVs;
}

struct PixelIn
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float2 tex2 : TEXCOORD1;
    float2 tex3 : TEXCOORD2;
};

Texture2D Diffuse : register(t0);
Texture2D Decals : register(t7);
Texture2D LayerDiffuse : register(t10);
SamplerState Sampler : register(s0);

float4 main(PixelIn input) : SV_TARGET
{
    float4 diffuseCol = Diffuse.Sample(Sampler, input.tex);
    float4 decalCol = Decals.Sample(Sampler, input.tex2);

    // if we have a layered texture bound, sample it
    if (hasLayeredUVs) {
        float4 layeredCol = LayerDiffuse.Sample(Sampler, input.tex3);
        float4 col = saturate(diffuseCol * layeredCol);

        return float4(col.xyz, diffuseCol.a);
    }

    return diffuseCol;
}