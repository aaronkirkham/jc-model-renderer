#include "helper.hlsl"

struct VertexIn
{
    min16int2 position : POSITION0;
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
    output.position = float4(unpack_s16(input.position.x), unpack_s16(input.position.y), 0, 1);
    output.colour = input.colour;

    return output;
}