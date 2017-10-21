float unpack_s16(min16int value)
{
    return (value * 1.0 / 32767);
}

float3 unpack_vec3(float value, bool is_colour)
{
    float3 result = float3(0, 0, 0);

    if (is_colour) {
        result.x = value;
        result.y = value / 256.0f;
        result.z = value / 65536.0f;
    }

    result.x -= floor(result.x);
    result.y -= floor(result.z);
    result.y -= floor(result.z);

    if (!is_colour) {
        result.x = (result.x * 2 - 1);
        result.y = (result.y * 2 - 1);
        result.z = (result.z * 2 - 1);
    }

    return result;
}