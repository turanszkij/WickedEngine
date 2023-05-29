#define DISABLE_SOFT_SHADOWMAP
#define TRANSPARENT_SHADOWMAP_SECONDARY_DEPTH_CHECK
#include "globals.hlsli"
#include "skyAtmosphere.hlsli"
#include "fogHF.hlsli"
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

void RenderAerialPerspective(uint3 DTid, float2 uv, float depth, float3 depthWorldPosition,	float3 rayOrigin, float3 rayDirection, inout float3 luminance, inout float transmittance, bool forceHighQuality)
{
	AtmosphereParameters atmosphere = GetWeather().atmosphere;

	float3 worldPosition = GetCameraPlanetPos(atmosphere, rayOrigin);
	float3 worldDirection = rayDirection;
	
	float viewHeight = length(worldPosition);

	// Switch to high quality when above atmosphere layer, if high quality is not already enabled:
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
		if (!MoveToTopAtmosphere(worldPosition, worldDirection, atmosphere.topRadius))
		{
			return;
		}
			
		// Apply the start offset after moving to the top of atmosphere to avoid black pixels
		worldPosition += worldDirection * AP_START_OFFSET_KM;
		
		float3 sunDirection = GetSunDirection();
		float3 sunIlluminance = GetSunColor();
		
		const bool receiveShadow = GetFrame().options & OPTION_BIT_REALISTIC_SKY_RECEIVE_SHADOW;

		const float tDepth = length((depthWorldPosition.xyz * M_TO_SKY_UNIT) - (worldPosition + atmosphere.planetCenter)); // apply earth offset to go back to origin as top of earth mode
		const float sampleCountIni = 0.0;
		const bool variableSampleCount = true;
#ifdef AERIALPERSPECTIVE_CAPTURE
		const bool perPixelNoise = false;
#else
		const bool perPixelNoise = GetFrame().options & OPTION_BIT_TEMPORALAA_ENABLED;
#endif // AERIALPERSPECTIVE_CAPTURE
		const bool opaque = true;
		const bool ground = false;
		const bool mieRayPhase = true;
		const bool multiScatteringApprox = true;
		const bool volumetricCloudShadow = receiveShadow;
		const bool opaqueShadow = receiveShadow;
		const float opticalDepthScale = atmosphere.aerialPerspectiveScale;
		SingleScatteringResult ss = IntegrateScatteredLuminance(
			atmosphere, DTid.xy, worldPosition, worldDirection, sunDirection, sunIlluminance, tDepth, sampleCountIni, variableSampleCount,
			perPixelNoise, opaque, ground, mieRayPhase, multiScatteringApprox, volumetricCloudShadow, opaqueShadow, texture_transmittancelut, texture_multiscatteringlut, opticalDepthScale);

		luminance = ss.L;
		transmittance = 1.0 - dot(ss.transmittance, float3(1.0 / 3.0, 1.0 / 3.0, 1.0 / 3.0));
	}

	// Since we apply opaque fog later in object shader, let's subtract the fog from aerial perspective
	// so it gets applied correctly, since fog has to place OVER aerial perspective.
	{
		float distance = length(depthWorldPosition.xyz - rayOrigin);		
		float fogAmount = GetFogAmount(distance, rayOrigin, rayDirection);
		
		luminance *= 1.0 - fogAmount;
		transmittance *= 1.0 - fogAmount;
	}
}

#ifdef AERIALPERSPECTIVE_CAPTURE
[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	TextureCube input = bindless_cubemaps[capture.texture_input];
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

	float4 composite = input.SampleLevel(sampler_linear_clamp, N, 0);

	// Ignore skybox
	if (depth == 0.0)
	{
		output[uint3(DTid.xy, DTid.z)] = composite;
		return;
	}

	float3 depthWorldPosition = reconstruct_position(uv, depth, GetCamera(DTid.z).inverse_view_projection);

	float3 rayOrigin = GetCamera(DTid.z).position;
	float3 rayDirection = normalize(N);	
	
	float3 luminance = float3(0.0, 0.0, 0.0);
	float transmittance = 0.0;
	RenderAerialPerspective(DTid, uv, depth, depthWorldPosition, rayOrigin, rayDirection, luminance, transmittance, true);

	float4 result = float4(luminance, transmittance);
	
    // Output
	output[uint3(DTid.xy, DTid.z)] = float4(composite.rgb * (1.0 - result.a) + result.rgb, composite.a * (1.0 - result.a));
}
#else
[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	float2 uv = (DTid.xy + 0.5f) * postprocess.resolution_rcp;
	
	float depth = texture_depth[DTid.xy];
	
	// Ignore skybox
	if (depth == 0.0)
	{
		texture_output[DTid.xy] = 0;
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
	RenderAerialPerspective(DTid, uv, depth, depthWorldPosition, rayOrigin, rayDirection, luminance, transmittance, false);
	
	float4 result = float4(luminance, transmittance);
	
    // Output
	texture_output[DTid.xy] = result;
}
#endif // AERIALPERSPECTIVE_CAPTURE
