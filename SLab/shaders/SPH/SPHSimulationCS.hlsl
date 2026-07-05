#include "SPHUtility.hlsli"

[numthreads(256, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    int i = DTid.x;
    float dt = gParticleLocalCB.dt;

    float minX = gParticleLocalCB.gGridMin.x;
    float minY = gParticleLocalCB.gGridMin.y;
    float minZ = gParticleLocalCB.gGridMin.z;

    float maxX = gParticleLocalCB.gGridMax.x;
    float maxZ = gParticleLocalCB.gGridMax.z;
    float maxY = gParticleLocalCB.gGridMax.y;

    if (i >= gParticleLocalCB.particleCount)
        return;

    float3 prev_vel = prev_particles[i].velocity;
    float3 prev_pos = prev_particles[i].position;

    curr_particles[i].velocity = prev_vel + curr_particles[i].acceleration * dt;
    curr_particles[i].position = prev_pos + curr_particles[i].velocity * dt;

    if (curr_particles[i].position.y <= minY && curr_particles[i].velocity.y < 0.f)
    {
        float over = minY - curr_particles[i].position.y;  // 박힌 깊이
        curr_particles[i].position.y = minY + over * 0.2f; // 반사 비율만큼 되돌리기
        curr_particles[i].velocity.y *= -0.2f;
    }
    if (curr_particles[i].position.y >= maxY && curr_particles[i].velocity.y > 0.f)
    {
        float over = abs(maxY - curr_particles[i].position.y); // 박힌 깊이
        curr_particles[i].position.y = maxY - over * 0.2f;     // 반사 비율만큼 되돌리기
        curr_particles[i].velocity.y *= -0.2;
    }

    // x축 경계조건
    if (curr_particles[i].position.x <= minX && curr_particles[i].velocity.x < 0.f)
    {
        float over = abs(minX - curr_particles[i].position.x); // 박힌 깊이
        curr_particles[i].position.x = minX + over * 0.2f;     // 반사 비율만큼 되돌리기
        curr_particles[i].velocity.x *= -0.2;
    }
    if (curr_particles[i].position.x >= maxX && curr_particles[i].velocity.x > 0.f)
    {
        float over = abs(maxX - curr_particles[i].position.x); // 박힌 깊이
        curr_particles[i].position.x = maxX - over * 0.2f;     // 반사 비율만큼 되돌리기
        curr_particles[i].velocity.x *= -0.2;
    }

    // z 축 경계조건
    if (curr_particles[i].position.z <= minZ && curr_particles[i].velocity.z < 0.f)
    {
        float over = abs(minZ - curr_particles[i].position.z); // 박힌 깊이
        curr_particles[i].position.z = minZ + over * 0.2f;     // 반사 비율만큼 되돌리기
        curr_particles[i].velocity.z *= -0.2;
    }
    if (curr_particles[i].position.z >= maxZ && curr_particles[i].velocity.z > 0.f)
    {
        float over = abs(maxZ - curr_particles[i].position.z); // 박힌 깊이
        curr_particles[i].position.z = maxZ - over * 0.2f;     // 반사 비율만큼 되돌리기
        curr_particles[i].velocity.z *= -0.2;
    }
}
