#include "globals.hlsli"
#include "ShaderInterop_Renderer.h"
#include "brdf.hlsli"

RWTEXTURE2D(output_velocity, float2, 0);
RWTEXTURE2D(output_depthbuffer, float, 1);

#ifdef VISIBILITY_MSAA
TEXTURE2DMS(texture_primitiveID, uint2, TEXSLOT_ONDEMAND0);
TEXTURE2DMS(texture_depthbuffer, float, TEXSLOT_ONDEMAND1);
RWTEXTURE2D(output_primitiveID, uint2, 3);
#else
TEXTURE2D(texture_primitiveID, uint2, TEXSLOT_ONDEMAND0);
TEXTURE2D(texture_depthbuffer, float, TEXSLOT_ONDEMAND1);
#endif // VISIBILITY_MSAA

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex, uint3 Gid : SV_GroupID, uint3 GTid : SV_GroupThreadID)
{
	uint2 pixel = DTid.xy;

	const float2 uv = ((float2)pixel + 0.5) * g_xFrame_InternalResolution_rcp;
	const float depth = texture_depthbuffer[pixel];
	output_depthbuffer[pixel] = depth;
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

	float4 pos2DPrev = mul(g_xCamera_PrevVP, float4(pre, 1));
	pos2DPrev.xy /= pos2DPrev.w;
	float2 pos2D = uv * 2 - 1;
	pos2D.y *= -1;
	float2 velocity = ((pos2DPrev.xy - g_xFrame_TemporalAAJitterPrev) - (pos2D.xy - g_xFrame_TemporalAAJitter)) * float2(0.5, -0.5);

	output_velocity[pixel] = velocity;
}
