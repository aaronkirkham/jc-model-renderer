#include "helper.hlsl"

// @gyp_compile(vs_5_0, vs_main)
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
    bool hasNormalMap;
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

VertexOut vs_main(VertexIn input)
{
    // unpack the vertex position
    float3 unpacked = float3(unpack_s16(input.position.x), unpack_s16(input.position.y), unpack_s16(input.position.z)) * scale;

    VertexOut output;
    output.position = mul(worldMatrix, float4(unpacked, 1));
    output.position = mul(viewProjection, output.position);
    output.tex = float2(unpack_s16(input.uvs.x) * uvExtent.x, unpack_s16(input.uvs.y) * uvExtent.y);
    output.normal = mul(float4(unpack_vec3(input.normal, false), 1), worldMatrix).xyz;
    output.tangent = mul(float4(unpack_vec3(input.tangent, false), 1), worldMatrix).xyz;
    output.colour = colour;

    return output;
}

// @gyp_compile(ps_5_0, ps_main)
cbuffer LightConstants : register(b0)
{
    float4 lightColour;
    float3 lightDirection;
}

Texture2D Diffuse : register(t0);
Texture2D Normal : register(t1);

SamplerState Sampler : register(s0);
SamplerState NormalMapSampler : register(s1);

float4 ps_main(VertexOut input) : SV_TARGET
{
    /*
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

        //input.normal = normalize(mul(normalMap, texSpace));
    }

    float3 colour = texColour * light_Ambient;
    colour += saturate(dot(light_Direction, input.normal) * light_Diffuse * texColour);

    return float4(colour, texColour.a);
    */

    return Diffuse.Sample(Sampler, input.tex);
}