struct PixelIn
{
    float4 position : SV_POSITION;
};

float4 main(PixelIn input) : SV_TARGET
{
    return float4(1, 1, 1, 1);
}