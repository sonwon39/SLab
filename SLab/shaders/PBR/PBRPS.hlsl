#include "PBRCommon.hlsli"

float4 main(PSInput input) : SV_TARGET
{
	const float3 Fdielectric = 0.04f;
	float3 worldPos = input.worldPos;
	
	SurfaceProperties surface;
	BRDFContext brdfContext;
	
	if (gMaterial.useNormalMap)
	{
		float3 N = normalize(input.normalW);
		float3 T = input.tangentW;
		T = normalize(T - dot(N, T) * N);
		float3 B = normalize(cross(N, T));
		float3x3 nMat = float3x3(T, B, N);

		float3 normalMap = gNormal.Sample(gWrapLinearSampler, input.uv).xyz;
		normalMap = normalMap * 2.f - 1.f;
		surface.N = normalize(mul(normalMap, nMat));
	}
	else
	{
		surface.N = normalize(input.normalW);
	}
	float3 N = surface.N;
	float3 IBL = 0.f;
	
	float metallic = gMaterial.useMetallicMap ? gMetallic.Sample(gWrapLinearSampler, input.uv).r : gMaterial.metallic;
	float roughness = gMaterial.useRoughnessMap ? gRoughness.Sample(gWrapLinearSampler, input.uv).r : gMaterial.roughness;
	float3 V = normalize(gGlobalCB.cameraPos - worldPos);
	
	
	float3 R = reflect(-V, N);

	surface.V = V;
	surface.c_diff = gAlbedo.Sample(gWrapLinearSampler, input.uv).rgb;
	surface.c_spec = gRadianceIBL.SampleLevel(gWrapLinearSampler, R, 5.f * roughness).rgb;
	surface.roughness = roughness;
	surface.metallic = metallic;
	surface.NoV = saturate(dot(N, V) + 1e-5);
	surface.F0 = lerp(Fdielectric, surface.c_diff, surface.metallic);

	brdfContext.NoV = surface.NoV;
	
	IBL += DiffuseIBL(surface);
	IBL += SpecularIBL(surface);

	float3 light = 0;
	float3 diffuse = Diffuse_Lambert(ComputeDiffuseAlbedo(surface));

	for (uint i = 0; i < MAX_LIGHT; i++)
	{
		Light l = gLight[i];
				
		if (l.enabled != 1)
			continue;

		float3 lPos = l.position;
		float3 radiance = l.radiance;
		float3 L = normalize(lPos - worldPos);
		float3 H = normalize(L + V);

		brdfContext.NoL = saturate(dot(N, L));
		brdfContext.VoL = saturate(dot(V, L));
		brdfContext.NoH = saturate(dot(N, H));
		brdfContext.VoH = saturate(dot(V, H));

		float3 specular_brdf = Specular_BRDF(surface, brdfContext);
		
		float r = l.fallOffEnd;
		float d = distance(lPos, worldPos);

		float window = Pow2(saturate(1 - Pow2(Pow2(d / r)))); // (1-(d/r)⁴)²
		float intensity = window / (d * d + 1);
		
		radiance = radiance * brdfContext.NoL * intensity;
		
		light += (diffuse + specular_brdf) * radiance;
	}
	return float4(light + IBL, 1.f);
}
