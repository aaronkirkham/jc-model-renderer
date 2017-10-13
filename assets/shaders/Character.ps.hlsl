struct PixelIn
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD;
};

Texture2D Diffuse : register(t0);

SamplerState DefaultSampler
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Border;
    AddressV = Border;
    BorderColor = float4(0, 0, 0, 0);
};

float4 main(PixelIn input) : SV_TARGET
{
    return float4(1, 1, 1, 1);
    //return Diffuse.Sample(DefaultSampler, input.tex);
}