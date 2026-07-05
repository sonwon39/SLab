#include "PBRCommon.hlsli"

PSInput main(VSInput input)
{
	PSInput output;
    
	float3 pos = input.position;
	
	float4 uv = float4(input.uv, 0.f, 1.f);
	uv = mul(uv, gMaterial.texTransform);

	if(gMaterial.useHeightMap)
	{
		pos += gMaterial.heightScale * gHeight.SampleLevel(gWrapLinearSampler, uv.xy, 0.f).x * input.normal;
	}
	
	float4 worldPos = mul(float4(pos, 1.f), gLocalCB.model);
	output.worldPos = worldPos.xyz;

	float4 svPos = mul(worldPos, gGlobalCB.view);
	svPos = mul(svPos, gGlobalCB.projection);

	output.normalW = normalize(mul(input.normal, (float3x3) gLocalCB.modelInvTranspose));
	output.tangentW = normalize(mul(input.tangent, (float3x3) gLocalCB.model));
		
	output.svPosition = svPos;
	output.uv = uv.xy;
    
	return output;
}
