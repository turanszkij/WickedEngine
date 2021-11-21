#include "globals.hlsli"
#include "ShaderInterop_Renderer.h"
#include "brdf.hlsli"

RWTexture2D<float2> output_velocity : register(u0);

RWTexture2D<float> output_depth_mip0 : register(u1);
RWTexture2D<float> output_depth_mip1 : register(u2);
RWTexture2D<float> output_depth_mip2 : register(u3);
RWTexture2D<float> output_depth_mip3 : register(u4);
RWTexture2D<float> output_depth_mip4 : register(u5);

RWTexture2D<float> output_lineardepth_mip0 : register(u6);
RWTexture2D<float> output_lineardepth_mip1 : register(u7);
RWTexture2D<float> output_lineardepth_mip2 : register(u8);
RWTexture2D<float> output_lineardepth_mip3 : register(u9);
RWTexture2D<float> output_lineardepth_mip4 : register(u10);

#ifdef VISIBILITY_MSAA
Texture2DMS<uint2> texture_primitiveID : register(t0);
Texture2DMS<float> texture_depthbuffer : register(t1);
RWTexture2D<uint2> output_primitiveID : register(u11);
#else
Texture2D<uint2> texture_primitiveID : register(t0);
Texture2D<float> texture_depthbuffer : register(t1);
#endif // VISIBILITY_MSAA

[numthreads(16, 16, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex, uint3 Gid : SV_GroupID, uint3 GTid : SV_GroupThreadID)
{
	uint2 pixel = DTid.xy;

	const float2 uv = ((float2)pixel + 0.5) * GetCamera().internal_resolution_rcp;
	const float depth = texture_depthbuffer[pixel];
	const float3 P = reconstruct_position(uv, depth);

	float3 pre = P;

	if (depth > 0)
	{
		uint2 primitiveID = texture_primitiveID[pixel];

#ifdef VISIBILITY_MSAA
		output_primitiveID[pixel] = primitiveID;
#endif // VISIBILITY_MSAA

		PrimitiveID prim;
		prim.unpack(primitiveID);

		Surface surface;
		if (surface.load(prim, P))
		{
			pre = surface.pre;
		}

	}

	float4 pos2DPrev = mul(GetCamera().previous_view_projection, float4(pre, 1));
	pos2DPrev.xy /= pos2DPrev.w;
	float2 pos2D = uv * 2 - 1;
	pos2D.y *= -1;
	float2 velocity = ((pos2DPrev.xy - GetCamera().temporalaa_jitter_prev) - (pos2D.xy - GetCamera().temporalaa_jitter)) * float2(0.5, -0.5);

	output_velocity[pixel] = velocity;



	// Downsample depths:
	output_depth_mip0[pixel] = depth;
	float lineardepth = compute_lineardepth(depth) * GetCamera().z_far_rcp;
	output_lineardepth_mip0[pixel] = lineardepth;

	if (GTid.x % 2 == 0 && GTid.y % 2 == 0)
	{
		output_depth_mip1[pixel / 2] = depth;
		output_lineardepth_mip1[pixel / 2] = lineardepth;
	}

	if (GTid.x % 4 == 0 && GTid.y % 4 == 0)
	{
		output_depth_mip2[pixel / 4] = depth;
		output_lineardepth_mip2[pixel / 4] = lineardepth;
	}

	if (GTid.x % 8 == 0 && GTid.y % 8 == 0)
	{
		output_depth_mip3[pixel / 8] = depth;
		output_lineardepth_mip3[pixel / 8] = lineardepth;
	}

	if (GTid.x % 16 == 0 && GTid.y % 16 == 0)
	{
		output_depth_mip4[pixel / 16] = depth;
		output_lineardepth_mip4[pixel / 16] = lineardepth;
	}

}
