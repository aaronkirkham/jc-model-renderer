cbuffer LightConstants : register(b0)
{
    float4 lightColour;
    float3 lightDirection;
}

cbuffer ModelConstants : register(b2)
{
    float scale;
    float2 uvExtent;
    bool hasNormalMap;
}

struct PixelIn
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float4 colour : COLOR;
};

Texture2D Diffuse : register(t0);
Texture2D Normal : register(t1);

SamplerState Sampler : register(s0);
SamplerState NormalMapSampler : register(s1);

float4 main(PixelIn input) : SV_TARGET
{
    float4 light_Ambient = float4(0.2f, 0.2f, 0.2f, 1.0f);
    float4 light_Diffuse = float4(1.0f, 1.0f, 1.0f, 1.0f);
    float3 light_Direction = float3(0.0f, -1.0f, 0.0f);

    input.normal = normalize(input.normal);

    float4 texColour = Diffuse.Sample(Sampler, input.tex);

    if (hasNormalMap) {
        float4 normalMap = Normal.Sample(NormalMapSampler, input.tex);

        normalMap = (normalMap * 2.0f) - 1.0f;

        input.tangent = normalize(input.tangent - dot(input.tangent, input.normal) * input.normal);

        float3 bitangent = cross(input.normal, input.tangent);
        float3x3 texSpace = float3x3(input.tangent, bitangent, input.normal);

        input.normal = normalize(mul(normalMap, texSpace));
    }

    float3 colour = texColour * light_Ambient;
    colour += saturate(dot(light_Direction, input.normal) * light_Diffuse * texColour);

    return float4(colour, texColour.a);
}