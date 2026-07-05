#include "PBRCommon.hlsli"

ConstantBuffer<SkinnedLocalConstant> gBoneTransformCB : register(b3);

PSInput main(SkinnedVSInput input)
{
	PSInput output;
    
	float2 uv = mul(float4(input.uv, 0, 1), gMaterial.texTransform).xy;
	output.uv = uv;
    
	uint indices[8] =
	{
		input.blendIndices0.x,
        input.blendIndices0.y,
        input.blendIndices0.z,
        input.blendIndices0.w,
        input.blendIndices1.x,
        input.blendIndices1.y,
        input.blendIndices1.z,
        input.blendIndices1.w
	};
	float weights[8] =
	{
		input.blendWeight0.x,
        input.blendWeight0.y,
        input.blendWeight0.z,
        input.blendWeight0.w,
        input.blendWeight1.x,
        input.blendWeight1.y,
        input.blendWeight1.z,
        input.blendWeight1.w
	};
    
	float3 posL = 0.f;
	float3 normL = 0.f;
	float3 tanL = 0.f;
	for (uint i = 0; i < 8; i++)
	{
		posL += weights[i] * mul(float4(input.pos, 1.f), gBoneTransformCB.boneTransform[indices[i]]).xyz;
		normL += weights[i] * mul(float4(input.normal, 0.f), gBoneTransformCB.boneTransform[indices[i]]).xyz;
		tanL += weights[i] * mul(float4(input.tangent, 0.f), gBoneTransformCB.boneTransform[indices[i]]).xyz;
	}
	float3 normal = normalize(normL);
   
	if (gMaterial.useHeightMap)
	{
		posL += gMaterial.heightScale * gHeight.SampleLevel(gWrapLinearSampler, uv.xy, 0.f).x * input.normal;
	}
	float4 worldPos = mul(float4(posL, 1.f), gLocalCB.model);
	output.worldPos = worldPos.xyz;

	float4 svPos = mul(worldPos, gGlobalCB.view);
	svPos = mul(svPos, gGlobalCB.projection);

	output.normalW = normalize(mul(normal, (float3x3) gLocalCB.modelInvTranspose));
	output.tangentW = normalize(mul(tanL, (float3x3) gLocalCB.model));
		
	output.svPosition = svPos;
	output.uv = uv.xy;
    
	return output;
}
