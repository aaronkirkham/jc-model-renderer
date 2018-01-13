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
    float2 uv0 : TEXCOORD0;
    float2 uv1 : TEXCOORD1;
    float2 uv2 : TEXCOORD2;
    float3 normal : NORMAL0;
    float3 tangent : TANGENT0;
    float3 binormal : BINORMAL0;
    float4 worldPosition : TEXCOORD3;
};

VertexOut main(VertexIn input)
{
    VertexOut output;

    float4 worldPosition = mul(worldMatrix, float4(input.position, 1));

    output.worldPosition = worldPosition;
    output.position = mul(viewProjection, worldPosition);

    output.uv0 = input.uv0;
    output.uv1 = input.uv1;
    output.uv2 = input.uv2;

    float3 normal = unpack_vec3(input.normal, false);
    output.normal = mul(normal, (float3x3)worldMatrix);

    float3 tangent = unpack_vec3(input.tangent, false);
    output.tangent = mul(tangent, (float3x3)worldMatrix);

    float3 binormal = cross(normal, tangent);
    output.binormal = mul(binormal, (float3x3)worldMatrix);

    return output;
}