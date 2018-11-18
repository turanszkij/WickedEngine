#include "imageHF.hlsli"

float4 main(VertexToPixelPostProcess PSIn) : SV_TARGET
{
	const float2 direction = xPPParams0.xy;
	const float mip = xPPParams0.z;

	float4 color = 0;
	for (uint i = 0; i < 9; ++i)
	{
		color += xTexture.SampleLevel(Sampler, PSIn.tex + direction * gaussianOffsets[i], mip) * gaussianWeightsNormalized[i];
	}
	return color;
}
