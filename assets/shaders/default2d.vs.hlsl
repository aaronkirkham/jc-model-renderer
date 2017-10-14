#include "helper.hlsl"

cbuffer FrameConstant : register(b0)
{
    float4x4 viewProjection;
}

struct VertexIn
{
    //min16int2 position : POSITION0;
    float4 position : POSITION0;
    float4 colour : COLOR0;
};

struct VertexOut
{
    float4 position : SV_POSITION;
    float4 colour : COLOR;
};

VertexOut main(VertexIn input)
{
    VertexOut output;
    //output.position = float4(unpack_s16(input.position.x), unpack_s16(input.position.y), 0, 1);


    float4x4 mat;
    mat[0] = float4(1, 0, 0, 0);
    mat[1] = float4(0, 1, 0, 0);
    mat[2] = float4(0, 0, 1, 0);
    mat[3] = float4(0, 0, 0, 1);

    //output.position = float4(input.position.xyz, 1);

    float4 pos = mul(float4(input.position.xyz, 1), mat);

    output.position = mul(viewProjection, pos);
    output.colour = input.colour;

    return output;
}