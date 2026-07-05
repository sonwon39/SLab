#define HLSL
#include "GlobalConstant.h"
#include "PBR/PBRHLSLCompat.h"

Texture2D						 gAlbedo			 : register(t0);
Texture2D						 gAmbient			 : register(t1);
Texture2D						 gHeight			 : register(t2);
Texture2D						 gMetallic			 : register(t3);
Texture2D						 gNormal			 : register(t4);
Texture2D						 gRoughness			 : register(t5);
								 					 
Texture2D						 gBRDF				 : register(t6);
TextureCube						 gIrradianceIBL		 : register(t7);
TextureCube						 gIBL				 : register(t8);
TextureCube						 gRadianceIBL        : register(t9);
													 
ConstantBuffer<GlobalConstant>	 gGlobalCB			 : register(b0);
ConstantBuffer<LocalConstant>	 gLocalCB			 : register(b1);
ConstantBuffer<MaterialConstant> gMaterial			 : register(b2);

StructuredBuffer<Light>			 gLight				 : register(t10);

SamplerState 					 gWrapLinearSampler  : register(s0);
SamplerState					 gClampLinearSampler : register(s1);

struct VSInput
{
	float3 position	: POSITION;
	float3 normal	: NORMAL;
	float3 tangent  : TANGENT;
	float2 uv		: TEXCOORD;
};

struct PSInput
{
	float4 svPosition   : SV_Position;
	float3 worldPos		: Position;
	float3 normalW		: NORMAL;
	float3 tangentW		: TANGENT;
	float2 uv 			: TEXCOORD;
};

struct SkinnedVSInput
{
	float3 pos : POSITION;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float2 uv : TEXCOORD;
	float4 blendWeight0 : BLENDWEIGHT0;
	float4 blendWeight1 : BLENDWEIGHT1;
	uint4 blendIndices0 : BLENDINDICES0;
	uint4 blendIndices1 : BLENDINDICES1;
};
struct SurfaceProperties
{
    float3 N;
    float3 V;
    float3 c_diff;
    float3 c_spec;
    float roughness;
	float metallic;
    float NoV;
	float3 F0;
};

struct BRDFContext
{
	half NoV;
	half NoL;
	half VoL;
	half NoH;
	half VoH;
};


float GGX(float k, float NoV);
float D_GGX(float a2, float NoH);
float SchlickGGX(BRDFContext c, float k);
float Vis_SmithJointApprox(float a, float NoV, float NoL);

float Pow5(float x)
{
    float xSq = x * x;
    return xSq * xSq * x;
}

float Pow2(float x)
{
    return x * x;
}

float3 Fresnel_Schlick(float3 F0, float3 F90, float cos)
{
	return lerp( F0 , F90,  Pow5(1- cos));
}

float3 ComputeDiffuseAlbedo(SurfaceProperties surface)
{
	return surface.c_diff - surface.c_diff * surface.metallic;
}

float3 DiffuseIBL(SurfaceProperties surface)
{
	float3 diffuse = ComputeDiffuseAlbedo(surface);
	float3 irradiance =  gIrradianceIBL.Sample(gWrapLinearSampler, surface.N).rgb;
	return diffuse * irradiance;
}

float3 SpecularIBL(SurfaceProperties surface)
{
	float2 EnvBRDF = gBRDF.SampleLevel(gClampLinearSampler, float2(surface.NoV, 1 - surface.roughness), 0.f).rg;
	return (surface.F0 * EnvBRDF.x + EnvBRDF.y) * surface.c_spec;
}

float3 Diffuse_Lambert(float3 diffuseColor)
{
	const float PI = 3.141592f;
	return diffuseColor * (1 / PI);
}

float3 Schlick_Specular_BRDF(SurfaceProperties surface, BRDFContext c)
{
	float3 specular_brdf;
	float a = Pow2(surface.roughness);
	float a2 = Pow2(a);
	float k = Pow2(surface.roughness+1) / 8;

	float D = D_GGX(a2, c.NoH);
	float G = SchlickGGX(c, k);
	float3 F = Fresnel_Schlick(surface.F0, 1.f, c.VoH);

	return (D * F * G) / max(1e-5, (4 * c.NoL * c.NoV));
}

float3 Specular_BRDF(SurfaceProperties surface, BRDFContext c)
{
	float3 specular_brdf;
	float a = Pow2(surface.roughness);
	float a2 = Pow2(a);
	
	float D = D_GGX(a2, c.NoH);
	float Vis = Vis_SmithJointApprox(a, c.NoV, c.NoL);
	float3 F = Fresnel_Schlick(surface.F0, 1.f, c.VoH);

	return (D * Vis) * F ;
}

float D_GGX(float a2, float NoH)
{
	const float PI = 3.141592f;
	float NoH2 = NoH * NoH;
	float b = (NoH2* (a2-1)+1);
	return a2 / (PI * b*b);
}

float Vis_SmithJointApprox(float a, float NoV, float NoL )
{            
    float Vis_SmithV = NoL * ( NoV * ( 1 - a ) + a );
    float Vis_SmithL = NoV * ( NoL * ( 1 - a ) + a );
    return 0.5 * rcp( Vis_SmithV + Vis_SmithL );
}

float SchlickGGX(BRDFContext c, float k)
{
	return GGX(k, c.NoV) * GGX(k, c.NoL);
}

float GGX(float k, float NoV)
{
	return NoV / (NoV * (1.0 - k) + k);
}
