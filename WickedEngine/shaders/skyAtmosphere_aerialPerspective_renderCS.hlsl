#include "globals.hlsli"
#include "skyAtmosphere.hlsli"
#include "ShaderInterop_Postprocess.h"

#ifdef AERIALPERSPECTIVE_CAPTURE
PUSHCONSTANT(capture, AerialPerspectiveCapturePushConstants);
#else
PUSHCONSTANT(postprocess, PostProcess);
#endif // AERIALPERSPECTIVE_CAPTURE

#ifdef AERIALPERSPECTIVE_CAPTURE

#ifdef MSAA
Texture2DMSArray<float> texture_input_depth_MSAA : register(t0);
#else
TextureCube<float> texture_input_depth : register(t0);
#endif // MSAA

#else
RWTexture2D<float4> texture_output : register(u0);
#endif // AERIALPERSPECTIVE_CAPTURE

void RenderAerialPerspective(uint3 DTid, float2 uv, float3 depthWorldPosition, float3 rayOrigin, float3 rayDirection, inout float3 luminance, inout float transmittance, bool forceHighQuality)
{
	AtmosphereParameters atmosphere = GetWeather().atmosphere;
	
	float3 worldPosition = GetCameraPlanetPos(atmosphere, rayOrigin);
	float3 worldDirection = rayDirection;

	float viewHeight = length(worldPosition);
	
	if (viewHeight < atmosphere.topRadius && !(GetFrame().options & OPTION_BIT_REALISTIC_SKY_HIGH_QUALITY) && !forceHighQuality)
	{
		float4 AP = GetAerialPerspectiveTransmittance(uv, depthWorldPosition, rayOrigin, texture_cameravolumelut);
		
		luminance = AP.rgb;
		transmittance = AP.a;
	}
	else
	{
		// Move to top atmosphere as the starting point for ray marching.
		// This is critical to be after the above to not disrupt above atmosphere tests and voxel selection.
		if (MoveToTopAtmosphere(worldPosition, worldDirection, atmosphere.topRadius))
		{
			// Apply the start offset after moving to the top of atmosphere to avoid black pixels
			const float startOffsetKm = 0.1; // 100m seems enough for long distances
			worldPosition += worldDirection * startOffsetKm;

			float3 sunDirection = GetSunDirection();
			float3 sunIlluminance = GetSunColor();

			SamplingParameters sampling;
			{
				sampling.variableSampleCount = true;
				sampling.sampleCountIni = 0.0f;
				sampling.rayMarchMinMaxSPP = float2(4, 14);
				sampling.distanceSPPMaxInv = 0.01;
#ifdef AERIALPERSPECTIVE_CAPTURE
				sampling.perPixelNoise = false;
#else
				sampling.perPixelNoise = true;
#endif
			}
			const float tDepth = length((depthWorldPosition.xyz * M_TO_SKY_UNIT) - (worldPosition + atmosphere.planetCenter)); // apply earth offset to go back to origin as top of earth mode
			const bool opaque = true;
			const bool ground = false;
			const bool mieRayPhase = true;
			const bool multiScatteringApprox = true;
			const bool volumetricCloudShadow = true;
			const bool opaqueShadow = true;
			SingleScatteringResult ss = IntegrateScatteredLuminance(
				atmosphere, DTid.xy, worldPosition, worldDirection, sunDirection, sunIlluminance, sampling, tDepth, opaque, ground,
				mieRayPhase, multiScatteringApprox, volumetricCloudShadow, opaqueShadow, texture_transmittancelut, texture_multiscatteringlut);

			luminance = ss.L;
			transmittance = 1.0 - dot(ss.transmittance, float3(1.0 / 3.0, 1.0 / 3.0, 1.0 / 3.0));
		}
	}
}

#ifdef AERIALPERSPECTIVE_CAPTURE
[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	TextureCubeArray input = bindless_cubearrays[capture.texture_input];
	RWTexture2DArray<float4> output = bindless_rwtextures2DArray[capture.texture_output];

	const float2 uv = (DTid.xy + 0.5) * capture.resolution_rcp;
	const float3 N = uv_to_cubemap(uv, DTid.z);

#ifdef MSAA
	float3 uv_slice = cubemap_to_uv(N);
	uint2 cube_dim;
	uint cube_elements;
	uint cube_sam;
	texture_input_depth_MSAA.GetDimensions(cube_dim.x, cube_dim.y, cube_elements, cube_sam);
	uv_slice.xy *= cube_dim;
	const float depth = texture_input_depth_MSAA.Load(uv_slice, 0);
#else
	const float depth = texture_input_depth.SampleLevel(sampler_point_clamp, N, 0).r;
#endif // MSAA

	float4 composite = input.SampleLevel(sampler_linear_clamp, float4(N, capture.arrayIndex), 0);

	// For Aerial Perspective we don't focus on sky
	if (depth == 0.0)
	{
		output[uint3(DTid.xy, DTid.z + capture.arrayIndex * 6)] = composite;
		return;
	}

	float3 depthWorldPosition = reconstruct_position(uv, depth, xCubemapRenderCams[DTid.z].inverse_view_projection);

	float3 rayOrigin = GetCamera().position;
	float3 rayDirection = normalize(N);	
	
	float3 luminance = float3(0.0, 0.0, 0.0);
	float transmittance = 0.0;
	RenderAerialPerspective(DTid, uv, depthWorldPosition, rayOrigin, rayDirection, luminance, transmittance, true);

	float4 result = float4(luminance, transmittance);
	
    // Output
	output[uint3(DTid.xy, DTid.z + capture.arrayIndex * 6)] = float4(composite.rgb * (1.0 - result.a) + result.rgb, composite.a * (1.0 - result.a));
}
#else
[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	// This is necessary for accurate upscaling. This is so we don't reuse the same half-res pixels
	uint2 offset = floor(blue_noise(uint2(0, 0)).xy);
	float2 uv = (offset + DTid.xy + 0.5f) * postprocess.resolution_rcp;
	
	const float depth = texture_depth.SampleLevel(sampler_point_clamp, uv, 0);

	// For Aerial Perspective we don't focus on sky
	if (depth == 0.0)
	{
		texture_output[DTid.xy] = float4(0.0, 0.0, 0.0, 0.0);
		return;
	}
	
	float2 screenPosition = uv_to_clipspace(uv);
	
	float4 unprojected = mul(GetCamera().inverse_view_projection, float4(screenPosition, 0, 1));
	unprojected.xyz /= unprojected.w;

	float3 depthWorldPosition = reconstruct_position(uv, depth);

	float3 rayOrigin = GetCamera().position;
	float3 rayDirection = normalize(unprojected.xyz - rayOrigin);
	
	float3 luminance = float3(0.0, 0.0, 0.0);
	float transmittance = 0.0;
	RenderAerialPerspective(DTid, uv, depthWorldPosition, rayOrigin, rayDirection, luminance, transmittance, false);

	float4 result = float4(luminance, transmittance);
	
    // Output
	texture_output[DTid.xy] = result;
}
#endif // AERIALPERSPECTIVE_CAPTURE
