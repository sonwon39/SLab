#include "ParticleCommon.hlsli"

StructuredBuffer<SPHParticle> particles : register(t0);

// GS 출력 제한 때문에 너무 크게 잡으면 컴파일 에러가 날 수 있음
#define LAT_SEG 4
#define LON_SEG 8

#define PI 3.14159265359f
#define TWO_PI 6.28318530718f

void EmitSphereVertex(float3 worldPos, float3 color, float2 uv, inout TriangleStream<PSInput> output)
{
    PSInput element = (PSInput)0;

    element.color = color;
    element.uv = uv;

    float4 p = float4(worldPos, 1.0f);
    p = mul(p, g_globalConstant.view);
    p = mul(p, g_globalConstant.projection);

    element.pos = p;

    output.Append(element);
}

[maxvertexcount(LAT_SEG * (LON_SEG + 1) * 2)] void main(point GSInput input[1], inout TriangleStream<PSInput> output)
{
    float3 center = input[0].pos;
    float r = input[0].radius;
    float3 color = input[0].color;

    for (uint lat = 0; lat < LAT_SEG; ++lat)
    {
        float v0 = (float)lat / LAT_SEG;
        float v1 = (float)(lat + 1) / LAT_SEG;

        // -PI/2 ~ +PI/2
        float theta0 = -PI * 0.5f + PI * v0;
        float theta1 = -PI * 0.5f + PI * v1;

        float y0 = sin(theta0);
        float y1 = sin(theta1);

        float xz0 = cos(theta0);
        float xz1 = cos(theta1);

        for (uint lon = 0; lon <= LON_SEG; ++lon)
        {
            float u = (float)lon / LON_SEG;
            float phi = TWO_PI * u;

            float c = cos(phi);
            float s = sin(phi);

            float3 local0 = float3(xz0 * c, y0, xz0 * s);
            float3 local1 = float3(xz1 * c, y1, xz1 * s);

            float3 world0 = center + local0 * r;
            float3 world1 = center + local1 * r;

            EmitSphereVertex(world0, color, float2(u, v0), output);
            EmitSphereVertex(world1, color, float2(u, v1), output);
        }

        output.RestartStrip();
    }
}
