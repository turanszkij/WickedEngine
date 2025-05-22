#define TRANSPARENT_SHADOWMAP_SECONDARY_DEPTH_CHECK // fix the lack of depth testing
#define DISABLE_SOFT_SHADOWMAP
#include "volumetricLightHF.hlsli"
#include "fogHF.hlsli"
#include "oceanSurfaceHF.hlsli"

float4 main(VertexToPixel input) : SV_TARGET
{
	//return float4(1,0,0,1);
	ShaderEntity light = load_entity(rectlights().first_item() + (uint)g_xColor.x);

	float2 ScreenCoord = input.pos2D.xy / input.pos2D.w * float2(0.5, -0.5) + 0.5;
	float4 depths = texture_depth.GatherRed(sampler_point_clamp, ScreenCoord);
	float depth = max(input.pos.z, max(depths.x, max(depths.y, max(depths.z, depths.w))));
	float3 P = reconstruct_position(ScreenCoord, depth);
	float3 nearP = GetCamera().frustum_corners.screen_to_nearplane(ScreenCoord);
	float3 V = nearP - P; // ortho support
	float cameraDistance = length(V);
	V /= cameraDistance;

	// Fix for ocean: because ocean is not in linear depth, we trace it instead
	const ShaderOcean ocean = GetWeather().ocean;
	if(ocean.IsValid() && V.y > 0)
	{
		float3 ocean_surface_pos = intersectPlaneClampInfinite(GetCamera().position, V, float3(0, 1, 0), ocean.water_height);
		float dist = distance(ocean_surface_pos, GetCamera().position);
		if(dist < cameraDistance)
		{
			P = ocean_surface_pos;
			cameraDistance = dist;
		}
	}

	float marchedDistance = 0;
	half3 accumulation = 0;

	float3 rayEnd = nearP;
	
	const uint sampleCount = 16;
	const half sampleCount_rcp = rcp((half)sampleCount);
	const float stepSize = distance(rayEnd, P) / sampleCount;

	// dither ray start to help with undersampling:
	P = P + V * stepSize * dither(input.pos.xy);
	
	const uint maskTex = light.GetTextureIndex();
	
	const half4 quaternion = light.GetQuaternion();
	const half3 right = rotate_vector(half3(1, 0, 0), quaternion);
	const half3 up = rotate_vector(half3(0, 1, 0), quaternion);
	const half3 forward = cross(up, right);
	const half light_length = max(0.01, light.GetLength());
	const half light_height = max(0.01, light.GetHeight());
	const float3 p0 = light.position - right * light_length * 0.5 + up * light_height * 0.5;
	const float3 p1 = light.position + right * light_length * 0.5 + up * light_height * 0.5;
	const float3 p2 = light.position + right * light_length * 0.5 - up * light_height * 0.5;
	const float3 p3 = light.position - right * light_length * 0.5 - up * light_height * 0.5;

	// Perform ray marching to integrate light volume along view ray:
	[loop]
	for (uint i = 0; i < sampleCount; ++i)
	{
		float3 L = light.position - P;
		const float3 Lunnormalized = L;
		const half dist2 = dot(L, L);
		const half dist = sqrt(dist2);
		L /= dist;

		const half range = light.GetRange();
		const half range2 = range * range;
		half3 attenuation = attenuation_pointlight(dist2, range, range2);
	
		if (dot(P - light.position, forward) <= 0)
			continue; // behind light
		
		// Solid angle based on the Frostbite presentation: Moving Frostbite to Physically Based Rendering by Sebastien Lagarde, Charles de Rousiers, Siggraph 2014
		float3 v0 = normalize(p0 - P);
		float3 v1 = normalize(p1 - P);
		float3 v2 = normalize(p2 - P);
		float3 v3 = normalize(p3 - P);
		float3 n0 = normalize(cross(v0, v1));
		float3 n1 = normalize(cross(v1, v2));
		float3 n2 = normalize(cross(v2, v3));
		float3 n3 = normalize(cross(v3, v0));
		float g0 = acos(dot(-n0, n1));
		float g1 = acos(dot(-n1, n2));
		float g2 = acos(dot(-n2, n3));
		float g3 = acos(dot(-n3, n0));
		const float solid_angle = saturate(g0 + g1 + g2 + g3 - 2 * PI);
	
		attenuation *= solid_angle * 0.25 * (
			saturate(dot(v0, L)) +
			saturate(dot(v1, L)) +
			saturate(dot(v2, L)) +
			saturate(dot(v3, L))
		);
		
		[branch]
		if (light.IsCastingShadow())
		{
			float4 shadow_pos = mul(load_entitymatrix(light.GetMatrixIndex() + 0), float4(P, 1));
			shadow_pos.xyz /= shadow_pos.w;
			float2 shadow_uv = shadow_pos.xy * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);
			[branch]
			if ((saturate(shadow_uv.x) == shadow_uv.x) && (saturate(shadow_uv.y) == shadow_uv.y))
			{
				attenuation *= shadow_2D(light, shadow_pos.z, shadow_uv.xy, 0);
			}
		}
			
		[branch]
		if (maskTex > 0)
		{
			float4 shadow_pos = mul(load_entitymatrix(light.GetMatrixIndex() + 0), float4(P, 1));
			shadow_pos.xyz /= shadow_pos.w;
			float2 shadow_uv = shadow_pos.xy * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);
			half4 mask = bindless_textures_half4[descriptor_index(maskTex)].Sample(sampler_linear_clamp, shadow_uv);
			attenuation *= mask.rgb * mask.a;
		}

		// Evaluate sample height for exponential fog calculation, given 0 for V:
		attenuation *= g_xColor.y + GetFogAmount(cameraDistance - marchedDistance, P, 0);
		attenuation *= ComputeScattering(saturate(dot(L, -V)));

		accumulation += attenuation;

		marchedDistance += stepSize;
		P = P + V * stepSize;
	}

	accumulation *= sampleCount_rcp;

	return max(0, half4(accumulation * light.GetColor().rgb, 1));
}
