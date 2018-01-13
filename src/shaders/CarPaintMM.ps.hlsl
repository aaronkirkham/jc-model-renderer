cbuffer LightConstants : register(b0)
{
    float3 lightPosition;
    float3 lightDirection;
    float4 lightDiffuse;
}

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
    float2 uv0 : TEXCOORD0;
    float2 uv1 : TEXCOORD1;
    float2 uv2 : TEXCOORD2;
    float3 normal : NORMAL0;
    float3 tangent : TANGENT0;
    float3 binormal : BINORMAL0;
    float4 worldPosition : TEXCOORD3;
};

Texture2D Diffuse : register(t0);
Texture2D Normals : register(t1);
Texture2D Decals : register(t7);
Texture2D LayerDiffuse : register(t10);

SamplerState Sampler : register(s0);

float4 main(PixelIn input) : SV_TARGET
{
    /*
    float4 bumpMap = Normals.Sample(Sampler, input.uv0);

    float3 pos = normalize(lightPosition.xyz - input.worldPosition.xyz);

    float3 bumpNormal = normalize((bumpMap.x * input.tangent) + (bumpMap.y * input.binormal) + (bumpMap.z * input.normal));
    float4 bumpIntensity = saturate(dot(pos, bumpNormal));

    float4 diffuseCol = Diffuse.Sample(Sampler, input.uv0);
    color *= diffuseCol;
    return color;
    */

    float4 diffuseCol = Diffuse.Sample(Sampler, input.uv0);
    float4 decalCol = Decals.Sample(Sampler, input.uv2);

    // if we have a layered texture bound, sample it
    if (hasLayeredUVs) {
        float4 layeredCol = LayerDiffuse.Sample(Sampler, input.uv2);
        float4 col = saturate(diffuseCol * layeredCol);

        return float4(col.xyz, diffuseCol.a) * colour;
    }

    return diffuseCol * colour;
}