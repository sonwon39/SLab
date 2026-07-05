RWTexture2D<float4> texture : register(u0);

[numthreads(4, 256, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    if (DTid.x > 1280 || DTid.y > 720)
        return;

    if (DTid.x > 1280 / 2)
    {
        texture[DTid.xy] = float4(1.f, 1.f, 0.f, 1.f);
    }
    else
    {
        texture[DTid.xy] = float4(1.f, 1.f, 1.f, 1.f);
    }
}
