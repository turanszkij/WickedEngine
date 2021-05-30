#ifndef WI_VOLUMETRICLIGHT_HF
#define WI_VOLUMETRICLIGHT_HF
#include "globals.hlsli"
#include "brdf.hlsli"
#include "lightingHF.hlsli"

struct VertexToPixel {
	float4 pos			: SV_POSITION;
	float4 pos2D		: POSITION2D;
};

// Mie scaterring approximated with Henyey-Greenstein phase function.
//	https://www.alexandre-pestana.com/volumetric-lights/
#define G_SCATTERING 0.66
float ComputeScattering(float lightDotView)
{
	float result = 1.0f - G_SCATTERING * G_SCATTERING;
	result /= (4.0f * PI * pow(1.0f + G_SCATTERING * G_SCATTERING - (2.0f * G_SCATTERING) * lightDotView, 1.5f));
	return result;
}

#endif // WI_VOLUMETRICLIGHT_HF
