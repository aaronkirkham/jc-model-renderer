struct PixelIn
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float2 tex2 : TEXCOORD1;
    float2 tex3 : TEXCOORD2;
};

Texture2D Diffuse : register(t0);
Texture2D Shared : register(t3);
SamplerState Sampler : register(s0);

float4 main(PixelIn input) : SV_TARGET
{
    float4 diffuseCol = Diffuse.Sample(Sampler, input.tex);
    float4 layeredCol = Shared.Sample(Sampler, input.tex3);

    // if we have a layered texture bound, sample it
    if (layeredCol.a > 0) {
        float4 col = saturate(diffuseCol * layeredCol * 1.6);
        return float4(col.xyz, diffuseCol.a);
    }

    return diffuseCol;
}