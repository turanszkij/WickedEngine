#define DISABLE_SOFT_SHADOWMAP
#define TRANSPARENT_SHADOWMAP_SECONDARY_DEPTH_CHECK
#include "globals.hlsli"
#include "skyAtmosphere.hlsli"

Texture2D<float4> transmittanceLUT : register(t0);
Texture2D<float4> multiScatteringLUT : register(t1);

RWTexture3D<float4> output : register(u0);
 
[numthreads(8, 8, 8)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	AtmosphereParameters atmosphere = GetWeather().atmosphere;

	float2 pixelPosition = float2(DTid.xy) + 0.5;
	float2 uv = pixelPosition * rcp(cameraVolumeLUTRes.xy);
	
	float2 screenPosition = uv_to_clipspace(uv);
	
	float4 unprojected = mul(GetCamera().inverse_view_projection, float4(screenPosition, 0, 1));
	unprojected.xyz /= unprojected.w;

	float3 rayOrigin = GetCamera().position;
	float3 rayDirection = normalize(unprojected.xyz - rayOrigin);

	float3 cameraPosition = GetCameraPlanetPos(atmosphere, rayOrigin);

	float3 worldPosition = cameraPosition;
	float3 worldDirection = rayDirection;
	

	float slice = ((float(DTid.z) + 0.5f) / AP_SLICE_COUNT);
	slice *= slice; // squared distribution
	slice *= AP_SLICE_COUNT;

	
	// Compute position from froxel information
	float tMax = AerialPerspectiveSliceToDepth(slice);
	float3 newWorldPosition = worldPosition + worldDirection * tMax;

	// If the voxel is under the ground, make sure to offset it out on the ground.
	float viewHeight = length(newWorldPosition);
	if (viewHeight <= (atmosphere.bottomRadius + PLANET_RADIUS_OFFSET))
	{
		// Apply a position offset to make sure no artefact are visible close to the earth boundaries for large voxel.
		newWorldPosition = normalize(newWorldPosition) * (atmosphere.bottomRadius + PLANET_RADIUS_OFFSET + 0.001f);
		worldDirection = normalize(newWorldPosition - cameraPosition);
		tMax = length(newWorldPosition - cameraPosition);
	}
	float tMaxMax = tMax;


	// Move ray marching start up to top atmosphere.
	viewHeight = length(worldPosition);
	if (viewHeight >= atmosphere.topRadius)
	{
		float3 prevWorldPosition = worldPosition;
		if (!MoveToTopAtmosphere(worldPosition, worldDirection, atmosphere.topRadius))
		{
			// Ray is not intersecting the atmosphere
			output[DTid] = float4(0.0, 0.0, 0.0, 1.0);
			return;
		}
		float lengthToAtmosphere = length(prevWorldPosition - worldPosition);
		if (tMaxMax < lengthToAtmosphere)
		{
			// tMaxMax for this voxel is not within earth atmosphere
			output[DTid] = float4(0.0, 0.0, 0.0, 1.0);
			return;
		}
		// Now world position has been moved to the atmosphere boundary: we need to reduce tMaxMax accordingly. 
		tMaxMax = max(0.0, tMaxMax - lengthToAtmosphere);
	}


	// Apply the start offset after moving to the top of atmosphere to avoid black pixels
	worldPosition += worldDirection * AP_START_OFFSET_KM;

	float3 sunDirection = GetSunDirection();
	float3 sunIlluminance = GetSunColor();

	const bool receiveShadow = GetFrame().options & OPTION_BIT_REALISTIC_SKY_RECEIVE_SHADOW;

	const float tDepth = 0.0;
	const float sampleCountIni = max(1.0, float(DTid.z + 1.0) * 2.0f); // Double sample count per slice
	const bool variableSampleCount = false;
	const bool perPixelNoise = false;
	const bool opaque = false;
	const bool ground = false;
	const bool mieRayPhase = true;
	const bool multiScatteringApprox = true;
	const bool volumetricCloudShadow = receiveShadow;
	const bool opaqueShadow = receiveShadow;
	const float opticalDepthScale = atmosphere.aerialPerspectiveScale;
	SingleScatteringResult ss = IntegrateScatteredLuminance(
        atmosphere, pixelPosition, worldPosition, worldDirection, sunDirection, sunIlluminance, tDepth, sampleCountIni, variableSampleCount,
		perPixelNoise, opaque, ground, mieRayPhase, multiScatteringApprox, volumetricCloudShadow, opaqueShadow, transmittanceLUT, multiScatteringLUT, opticalDepthScale, tMaxMax);

	const float transmittance = dot(ss.transmittance, float3(1.0f / 3.0f, 1.0f / 3.0f, 1.0f / 3.0f));
	output[DTid] = float4(ss.L, 1.0 - transmittance);
}
