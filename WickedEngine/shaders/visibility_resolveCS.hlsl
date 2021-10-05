#include "globals.hlsli"
#include "ShaderInterop_Renderer.h"
#include "brdf.hlsli"

RWTEXTURE2D(output_velocity, float2, 0);

RWTEXTURE2D(output_depth_mip0, float, 1);
RWTEXTURE2D(output_depth_mip1, float, 2);
RWTEXTURE2D(output_depth_mip2, float, 3);
RWTEXTURE2D(output_depth_mip3, float, 4);
RWTEXTURE2D(output_depth_mip4, float, 5);

RWTEXTURE2D(output_lineardepth_mip0, float, 6);
RWTEXTURE2D(output_lineardepth_mip1, float, 7);
RWTEXTURE2D(output_lineardepth_mip2, float, 8);
RWTEXTURE2D(output_lineardepth_mip3, float, 9);
RWTEXTURE2D(output_lineardepth_mip4, float, 10);

#ifdef VISIBILITY_MSAA
TEXTURE2DMS(texture_primitiveID, uint2, TEXSLOT_ONDEMAND0);
TEXTURE2DMS(texture_depthbuffer, float, TEXSLOT_ONDEMAND1);
RWTEXTURE2D(output_primitiveID, uint2, 11);
#else
TEXTURE2D(texture_primitiveID, uint2, TEXSLOT_ONDEMAND0);
TEXTURE2D(texture_depthbuffer, float, TEXSLOT_ONDEMAND1);
#endif // VISIBILITY_MSAA

[numthreads(16, 16, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex, uint3 Gid : SV_GroupID, uint3 GTid : SV_GroupThreadID)
{
	uint2 pixel = DTid.xy;

	const float2 uv = ((float2)pixel + 0.5) * GetCamera().InternalResolution_rcp;
	const float depth = texture_depthbuffer[pixel];
	const float3 P = reconstructPosition(uv, depth);

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

	float4 pos2DPrev = mul(GetCamera().PrevVP, float4(pre, 1));
	pos2DPrev.xy /= pos2DPrev.w;
	float2 pos2D = uv * 2 - 1;
	pos2D.y *= -1;
	float2 velocity = ((pos2DPrev.xy - GetCamera().TemporalAAJitterPrev) - (pos2D.xy - GetCamera().TemporalAAJitter)) * float2(0.5, -0.5);

	output_velocity[pixel] = velocity;



	// Downsample depths:
	output_depth_mip0[pixel] = depth;
	float lineardepth = getLinearDepth(depth) * GetCamera().ZFarP_rcp;
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
