#include "postProcessHF.hlsli"


float4 main(VertextoPixel PSIn) : SV_TARGET
{
	//float4 color = float4(0,0,0,0);

	//color += xTexture.SampleLevel(Sampler,PSIn.tex,0);

	//color=saturate((color - xBloom.y) / (1 - xBloom.y));
	//float grey = dot(color, float3(0.3, 0.59, 0.11));

	//return lerp(color,grey,xBloom.z);
	
	return clamp(xTexture.SampleLevel(Sampler,PSIn.tex,0)-1, 0, inf);
}