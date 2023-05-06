#include "globals.hlsli"
#include "ShaderInterop_Renderer.h"
#include "surfaceHF.hlsli"
#include "raytracingHF.hlsli"

RWTexture2D<float2> output_velocity : register(u0);

[numthreads(8, 8, 1)]
void main(uint2 DTid : SV_DispatchThreadID)
{
	uint2 pixel = DTid.xy;
	[branch]
	if (!GetCamera().is_pixel_inside_scissor(pixel))
	{
		output_velocity[pixel] = 0;
		return;
	}

	const float2 uv = ((float2)pixel + 0.5) * GetCamera().internal_resolution_rcp;
	const float2 clipspace = uv_to_clipspace(uv);
	RayDesc ray = CreateCameraRay(clipspace);

	uint primitiveID = texture_primitiveID[pixel];

	float3 pre;
	[branch]
	if (any(primitiveID))
	{
		PrimitiveID prim;
		prim.unpack(primitiveID);

		Surface surface;
		surface.init();

		[branch]
		if (surface.load(prim, ray.Origin, ray.Direction))
		{
			pre = surface.pre;
		}
	}
	else
	{
		pre = ray.Origin + ray.Direction * GetCamera().z_far;
	}

	float2 pos2D = clipspace;
	float4 pos2DPrev = mul(GetCamera().previous_view_projection, float4(pre, 1));
	pos2DPrev.xy /= max(0.0001, pos2DPrev.w); // max: avoid nan
	float2 velocity = ((pos2DPrev.xy - GetCamera().temporalaa_jitter_prev) - (pos2D.xy - GetCamera().temporalaa_jitter)) * float2(0.5, -0.5);
	output_velocity[pixel] = velocity;

}
