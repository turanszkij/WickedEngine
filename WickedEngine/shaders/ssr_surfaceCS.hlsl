#include "globals.hlsli"
#include "brdf.hlsli"
#include "lightingHF.hlsli"
#include "ShaderInterop_Postprocess.h"

PUSHCONSTANT(postprocess, PostProcess);

RWTexture2D<float3> output_surface_normal : register(u0);
RWTexture2D<float> output_surface_roughness : register(u1);
RWTexture2D<float3> output_surface_environment : register(u2);

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	uint2 dim;
	texture_depth.GetDimensions(dim.x, dim.y);

	float2 uv = (DTid.xy + 0.5f) / dim;

	float depth = texture_depth[DTid.xy];

	uint2 primitiveID = texture_gbuffer0[DTid.xy]; // Map to resolution

	PrimitiveID prim;
	prim.unpack(primitiveID);

	Surface surface;
	surface.init();
	if (!surface.load(prim, reconstruct_position(uv, depth)))
	{
		output_surface_normal[DTid.xy] = 0.0;
		output_surface_roughness[DTid.xy] = 0.0;
		output_surface_environment[DTid.xy] = 0.0;
		return;
	}

	float3 N = surface.N;
	float roughness = surface.roughness;
	float3 environmentReflection = EnvironmentReflection_Global(surface);

	output_surface_normal[DTid.xy] = N;
	output_surface_roughness[DTid.xy] = roughness;
	output_surface_environment[DTid.xy] = environmentReflection;
}
