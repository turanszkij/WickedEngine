#include "objectHF.hlsli"
#include "voxelHF.hlsli"
#include "volumetricCloudsHF.hlsli"

// Note: the voxelizer uses an overall simplified material and lighting model (no normal maps, only diffuse light and emissive)

#ifdef VOXELIZATION_CONSERVATIVE_RASTERIZATION_ENABLED
// Minimal axis-aligned bounding box used for the conservative-rasterization
// voxel/triangle overlap test in main(). The geometry shader fattens the
// triangle so the rasterizer covers every touched voxel; this test then rejects
// covered voxels that the (unexpanded) triangle does not actually overlap.
// These are local copies of the same helpers in lightCullingCS.hlsl.
struct AABB
{
	float3 c; // center
	float3 e; // half extents
};
bool IntersectAABB(AABB a, AABB b)
{
	if (abs(a.c[0] - b.c[0]) > (a.e[0] + b.e[0]))
		return false;
	if (abs(a.c[1] - b.c[1]) > (a.e[1] + b.e[1]))
		return false;
	if (abs(a.c[2] - b.c[2]) > (a.e[2] + b.e[2]))
		return false;
	return true;
}
void AABBfromMinMax(inout AABB aabb, float3 _min, float3 _max)
{
	aabb.c = (_min + _max) * 0.5f;
	aabb.e = abs(_max - aabb.c);
}
#endif // VOXELIZATION_CONSERVATIVE_RASTERIZATION_ENABLED

Texture3D<float4> input_previous_radiance : register(t0);

RWTexture3D<uint> output_atomic : register(u0);

void VoxelAtomicAverage(inout RWTexture3D<uint> output, in uint3 dest, in float4 color)
{
	float4 addingColor = float4(color.rgb, 1);
	uint newValue = PackVoxelColor(float4(addingColor.rgb, 1.0 / MAX_VOXEL_ALPHA));
	uint expectedValue = 0;
	uint actualValue;

	InterlockedCompareExchange(output[dest], expectedValue, newValue, actualValue);
	while (actualValue != expectedValue)
	{
		expectedValue = actualValue;

		color = UnpackVoxelColor(actualValue);
		color.a *= MAX_VOXEL_ALPHA;

		color.rgb *= color.a;

		color += addingColor;

		color.rgb /= color.a;

		color.a /= MAX_VOXEL_ALPHA;
		newValue = PackVoxelColor(color);

		InterlockedCompareExchange(output[dest], expectedValue, newValue, actualValue);
	}
}

// Note: centroid interpolation is used to avoid floating voxels in some cases
struct PSInput
{
	float4 pos : SV_POSITION;
	centroid float4 uvsets : UVSETS;
	centroid half4 color : COLOR;
	centroid float3 N : NORMAL;
	centroid float3 P : POSITION3D;

#ifdef VOXELIZATION_CONSERVATIVE_RASTERIZATION_ENABLED
	nointerpolation float3 aabb_min : AABB_MIN;
	nointerpolation float3 aabb_max : AABB_MAX;
#endif // VOXELIZATION_CONSERVATIVE_RASTERIZATION_ENABLED
	
	inline float4 GetUVSets()
	{
		float4 ret = uvsets;
		ret.xy = mad(ret.xy, GetMaterial().texMulAdd.xy, GetMaterial().texMulAdd.zw);
		return ret;
	}
};

void main(PSInput input)
{
	ShaderMaterial material = GetMaterial();

	float4 uvsets = input.GetUVSets();
	float3 P = input.P;

	VoxelClipMap clipmap = GetFrame().vxgi.clipmaps[g_xVoxelizer.clipmap_index];
	float3 uvw = GetFrame().vxgi.world_to_clipmap(P, clipmap);
	if (!is_saturated(uvw))
		return;

#ifdef VOXELIZATION_CONSERVATIVE_RASTERIZATION_ENABLED
	uint3 clipmap_pixel = uvw * GetFrame().vxgi.resolution;
	float3 clipmap_uvw_center = (clipmap_pixel + 0.5) * GetFrame().vxgi.resolution_rcp;
	float3 voxel_center = GetFrame().vxgi.clipmap_to_world(clipmap_uvw_center, clipmap);
	AABB voxel_aabb;
	voxel_aabb.c = voxel_center;
	voxel_aabb.e = clipmap.voxelSize;
	AABB triangle_aabb;
	AABBfromMinMax(triangle_aabb, input.aabb_min, input.aabb_max);
	if (!IntersectAABB(voxel_aabb, triangle_aabb))
		return;
#endif // VOXELIZATION_CONSERVATIVE_RASTERIZATION_ENABLED

	float4 baseColor = input.color;
	[branch]
	if (material.textures[BASECOLORMAP].IsValid() && (GetFrame().options & OPTION_BIT_DISABLE_ALBEDO_MAPS) == 0)
	{
		float lod_bias = 0;
		if (material.IsTransparent() || material.GetAlphaTest() > 0)
		{
			// If material is non opaque, then we apply bias to avoid sampling such a low
			//	mip level in which alpha is completely gone (helps with trees)
			lod_bias = -10;
		}
		baseColor *= material.textures[BASECOLORMAP].SampleBias(sampler_linear_wrap, uvsets, lod_bias);
	}

	float3 emissiveColor = material.GetEmissive();
	[branch]
	if (any(emissiveColor) && material.textures[EMISSIVEMAP].IsValid())
	{
		float4 emissiveMap = material.textures[EMISSIVEMAP].Sample(sampler_linear_wrap, uvsets);
		emissiveColor *= emissiveMap.rgb * emissiveMap.a;
	}

	float3 N = normalize(input.N);

	Lighting lighting;
	lighting.create(0, 0, 0, 0);

	Surface surface;
	surface.init();
	surface.P = P;
	surface.N = N;
	surface.create(material, baseColor, 0, 0);
	surface.roughness = material.GetRoughness();
	surface.sss = material.GetSSS();
	surface.sss_inv = material.GetSSSInverse();
	surface.layerMask = material.layerMask;
	surface.update();

	[branch]
	if (!directional_lights().empty())
	{
		ShaderEntityIterator iterator = directional_lights();
		for (uint entity_index = iterator.first_item(); entity_index < iterator.end_item(); ++entity_index)
		{
			ShaderEntity light = load_entity(entity_index);
			if (light.IsStaticLight())
				continue;
			light_directional(light, surface, lighting);
		}
	}

	[branch]
	if (!spotlights().empty())
	{
		ShaderEntityIterator iterator = spotlights();
		for (uint entity_index = iterator.first_item(); entity_index < iterator.end_item(); ++entity_index)
		{
			ShaderEntity light = load_entity(entity_index);
			if (light.IsStaticLight())
				continue;
			light_spot(light, surface, lighting);
		}
	}

	[branch]
	if (!pointlights().empty())
	{
		ShaderEntityIterator iterator = pointlights();
		for (uint entity_index = iterator.first_item(); entity_index < iterator.end_item(); ++entity_index)
		{
			ShaderEntity light = load_entity(entity_index);
			if (light.IsStaticLight())
				continue;
			light_point(light, surface, lighting);
		}
	}

	[branch]
	if (!rectlights().empty())
	{
		ShaderEntityIterator iterator = rectlights();
		for (uint entity_index = iterator.first_item(); entity_index < iterator.end_item(); ++entity_index)
		{
			ShaderEntity light = load_entity(entity_index);
			if (light.IsStaticLight())
				continue;
			light_rect(light, surface, lighting);
		}
	}

	// Self-emitted radiance (direct lighting + emissive), precomputed per
	// fragment so the temporal pass only needs to add the indirect bounce
	// (albedo * indirect). Mirrors the former temporal combine: baseColor.rgb *
	// directLight / PI + emissive
	float3 self_radiance = baseColor.rgb * (lighting.direct.diffuse / PI) + emissiveColor;

	// output:
	uint3 writecoord = floor(uvw * GetFrame().vxgi.resolution);
	writecoord.z *= VOXELIZATION_CHANNEL_COUNT; // de-interleaved channels

	float3 aniso_direction = N;

#if 0
	// This voxelization is faster but less accurate:
	uint face_offset = cubemap_to_uv(aniso_direction).z * GetFrame().vxgi.resolution;
	float4 baseColor_direction = baseColor;
	float3 selfRadiance_direction = self_radiance;
	float2 normal_direction = encode_oct(N) * 0.5 + 0.5;
	InterlockedAdd(output_atomic[writecoord + uint3(face_offset, 0, VOXELIZATION_CHANNEL_BASECOLOR_R)], PackVoxelChannel(baseColor_direction.r));
	InterlockedAdd(output_atomic[writecoord + uint3(face_offset, 0, VOXELIZATION_CHANNEL_BASECOLOR_G)], PackVoxelChannel(baseColor_direction.g));
	InterlockedAdd(output_atomic[writecoord + uint3(face_offset, 0, VOXELIZATION_CHANNEL_BASECOLOR_B)], PackVoxelChannel(baseColor_direction.b));
	InterlockedAdd(output_atomic[writecoord + uint3(face_offset, 0, VOXELIZATION_CHANNEL_BASECOLOR_A)], PackVoxelChannel(baseColor_direction.a));
	InterlockedAdd(output_atomic[writecoord + uint3(face_offset, 0, VOXELIZATION_CHANNEL_SELFRADIANCE_R)], PackVoxelChannel(selfRadiance_direction.r));
	InterlockedAdd(output_atomic[writecoord + uint3(face_offset, 0, VOXELIZATION_CHANNEL_SELFRADIANCE_G)], PackVoxelChannel(selfRadiance_direction.g));
	InterlockedAdd(output_atomic[writecoord + uint3(face_offset, 0, VOXELIZATION_CHANNEL_SELFRADIANCE_B)], PackVoxelChannel(selfRadiance_direction.b));
	InterlockedAdd(output_atomic[writecoord + uint3(face_offset, 0, VOXELIZATION_CHANNEL_NORMAL_R)], PackVoxelChannel(normal_direction.r));
	InterlockedAdd(output_atomic[writecoord + uint3(face_offset, 0, VOXELIZATION_CHANNEL_NORMAL_G)], PackVoxelChannel(normal_direction.g));
	InterlockedAdd(output_atomic[writecoord + uint3(face_offset, 0, VOXELIZATION_CHANNEL_FRAGMENT_COUNTER)], 1);

#else
	// This is slower but more accurate voxelization, by weighted voxel writes into multiple directions:
	float3 face_offsets = float3(
		aniso_direction.x > 0 ? 0 : 1,
		aniso_direction.y > 0 ? 2 : 3,
		aniso_direction.z > 0 ? 4 : 5
		) * GetFrame().vxgi.resolution;
	float3 direction_weights = abs(N);

	if (direction_weights.x > 0)
	{
		float4 baseColor_direction = baseColor * direction_weights.x;
		float3 selfRadiance_direction = self_radiance * direction_weights.x;
		float2 normal_direction = encode_oct(N * direction_weights.x) * 0.5 + 0.5;
		InterlockedAdd(output_atomic[writecoord + uint3(face_offsets.x, 0, VOXELIZATION_CHANNEL_BASECOLOR_R)], PackVoxelChannel(baseColor_direction.r));
		InterlockedAdd(output_atomic[writecoord + uint3(face_offsets.x, 0, VOXELIZATION_CHANNEL_BASECOLOR_G)], PackVoxelChannel(baseColor_direction.g));
		InterlockedAdd(output_atomic[writecoord + uint3(face_offsets.x, 0, VOXELIZATION_CHANNEL_BASECOLOR_B)], PackVoxelChannel(baseColor_direction.b));
		InterlockedAdd(output_atomic[writecoord + uint3(face_offsets.x, 0, VOXELIZATION_CHANNEL_BASECOLOR_A)], PackVoxelChannel(baseColor_direction.a));
		InterlockedAdd(output_atomic[writecoord + uint3(face_offsets.x, 0, VOXELIZATION_CHANNEL_SELFRADIANCE_R)], PackVoxelChannel(selfRadiance_direction.r));
		InterlockedAdd(output_atomic[writecoord + uint3(face_offsets.x, 0, VOXELIZATION_CHANNEL_SELFRADIANCE_G)], PackVoxelChannel(selfRadiance_direction.g));
		InterlockedAdd(output_atomic[writecoord + uint3(face_offsets.x, 0, VOXELIZATION_CHANNEL_SELFRADIANCE_B)], PackVoxelChannel(selfRadiance_direction.b));
		InterlockedAdd(output_atomic[writecoord + uint3(face_offsets.x, 0, VOXELIZATION_CHANNEL_NORMAL_R)], PackVoxelChannel(normal_direction.r));
		InterlockedAdd(output_atomic[writecoord + uint3(face_offsets.x, 0, VOXELIZATION_CHANNEL_NORMAL_G)], PackVoxelChannel(normal_direction.g));
		InterlockedAdd(output_atomic[writecoord + uint3(face_offsets.x, 0, VOXELIZATION_CHANNEL_FRAGMENT_COUNTER)], 1);
	}
	if (direction_weights.y > 0)
	{
		float4 baseColor_direction = baseColor * direction_weights.y;
		float3 selfRadiance_direction = self_radiance * direction_weights.y;
		float2 normal_direction = encode_oct(N * direction_weights.y) * 0.5 + 0.5;
		InterlockedAdd(output_atomic[writecoord + uint3(face_offsets.y, 0, VOXELIZATION_CHANNEL_BASECOLOR_R)], PackVoxelChannel(baseColor_direction.r));
		InterlockedAdd(output_atomic[writecoord + uint3(face_offsets.y, 0, VOXELIZATION_CHANNEL_BASECOLOR_G)], PackVoxelChannel(baseColor_direction.g));
		InterlockedAdd(output_atomic[writecoord + uint3(face_offsets.y, 0, VOXELIZATION_CHANNEL_BASECOLOR_B)], PackVoxelChannel(baseColor_direction.b));
		InterlockedAdd(output_atomic[writecoord + uint3(face_offsets.y, 0, VOXELIZATION_CHANNEL_BASECOLOR_A)], PackVoxelChannel(baseColor_direction.a));
		InterlockedAdd(output_atomic[writecoord + uint3(face_offsets.y, 0, VOXELIZATION_CHANNEL_SELFRADIANCE_R)], PackVoxelChannel(selfRadiance_direction.r));
		InterlockedAdd(output_atomic[writecoord + uint3(face_offsets.y, 0, VOXELIZATION_CHANNEL_SELFRADIANCE_G)], PackVoxelChannel(selfRadiance_direction.g));
		InterlockedAdd(output_atomic[writecoord + uint3(face_offsets.y, 0, VOXELIZATION_CHANNEL_SELFRADIANCE_B)], PackVoxelChannel(selfRadiance_direction.b));
		InterlockedAdd(output_atomic[writecoord + uint3(face_offsets.y, 0, VOXELIZATION_CHANNEL_NORMAL_R)], PackVoxelChannel(normal_direction.r));
		InterlockedAdd(output_atomic[writecoord + uint3(face_offsets.y, 0, VOXELIZATION_CHANNEL_NORMAL_G)], PackVoxelChannel(normal_direction.g));
		InterlockedAdd(output_atomic[writecoord + uint3(face_offsets.y, 0, VOXELIZATION_CHANNEL_FRAGMENT_COUNTER)], 1);
	}
	if (direction_weights.z > 0)
	{
		float4 baseColor_direction = baseColor * direction_weights.z;
		float3 selfRadiance_direction = self_radiance * direction_weights.z;
		float2 normal_direction = encode_oct(N * direction_weights.z) * 0.5 + 0.5;
		InterlockedAdd(output_atomic[writecoord + uint3(face_offsets.z, 0, VOXELIZATION_CHANNEL_BASECOLOR_R)], PackVoxelChannel(baseColor_direction.r));
		InterlockedAdd(output_atomic[writecoord + uint3(face_offsets.z, 0, VOXELIZATION_CHANNEL_BASECOLOR_G)], PackVoxelChannel(baseColor_direction.g));
		InterlockedAdd(output_atomic[writecoord + uint3(face_offsets.z, 0, VOXELIZATION_CHANNEL_BASECOLOR_B)], PackVoxelChannel(baseColor_direction.b));
		InterlockedAdd(output_atomic[writecoord + uint3(face_offsets.z, 0, VOXELIZATION_CHANNEL_BASECOLOR_A)], PackVoxelChannel(baseColor_direction.a));
		InterlockedAdd(output_atomic[writecoord + uint3(face_offsets.z, 0, VOXELIZATION_CHANNEL_SELFRADIANCE_R)], PackVoxelChannel(selfRadiance_direction.r));
		InterlockedAdd(output_atomic[writecoord + uint3(face_offsets.z, 0, VOXELIZATION_CHANNEL_SELFRADIANCE_G)], PackVoxelChannel(selfRadiance_direction.g));
		InterlockedAdd(output_atomic[writecoord + uint3(face_offsets.z, 0, VOXELIZATION_CHANNEL_SELFRADIANCE_B)], PackVoxelChannel(selfRadiance_direction.b));
		InterlockedAdd(output_atomic[writecoord + uint3(face_offsets.z, 0, VOXELIZATION_CHANNEL_NORMAL_R)], PackVoxelChannel(normal_direction.r));
		InterlockedAdd(output_atomic[writecoord + uint3(face_offsets.z, 0, VOXELIZATION_CHANNEL_NORMAL_G)], PackVoxelChannel(normal_direction.g));
		InterlockedAdd(output_atomic[writecoord + uint3(face_offsets.z, 0, VOXELIZATION_CHANNEL_FRAGMENT_COUNTER)], 1);
	}
#endif


//#if 0
//	uint face_offset = cubemap_to_uv(aniso_direction).z * GetFrame().vxgi.resolution;
//	VoxelAtomicAverage(output_radiance, writecoord + uint3(face_offset, 0, 0), color);
//	VoxelAtomicAverage(output_opacity, writecoord + uint3(face_offset, 0, 0), color.aaaa);
//#else
//	float3 face_offsets = float3(
//		aniso_direction.x > 0 ? 0 : 1,
//		aniso_direction.y > 0 ? 2 : 3,
//		aniso_direction.z > 0 ? 4 : 5
//		) * GetFrame().vxgi.resolution;
//	float3 direction_weights = abs(N);
//	VoxelAtomicAverage(output_radiance, writecoord + uint3(face_offsets.x, 0, 0), color * direction_weights.x);
//	VoxelAtomicAverage(output_radiance, writecoord + uint3(face_offsets.y, 0, 0), color * direction_weights.y);
//	VoxelAtomicAverage(output_radiance, writecoord + uint3(face_offsets.z, 0, 0), color * direction_weights.z);
//	VoxelAtomicAverage(output_opacity, writecoord + uint3(face_offsets.x, 0, 0), color.aaaa * direction_weights.x);
//	VoxelAtomicAverage(output_opacity, writecoord + uint3(face_offsets.y, 0, 0), color.aaaa * direction_weights.y);
//	VoxelAtomicAverage(output_opacity, writecoord + uint3(face_offsets.z, 0, 0), color.aaaa * direction_weights.z);
//#endif

	//bool done = false;
	//while (!done)
	//{
	//	// acquire lock:
	//	uint locked;
	//	InterlockedCompareExchange(lock[writecoord], 0, 1, locked);
	//	if (locked == 0)
	//	{
	//		float4 average = output_albedo[writecoord];
	//		float3 average_normal = output_normal[writecoord];

	//		average.a += 1;
	//		average.rgb += color.rgb;
	//		average_normal.rgb += N * 0.5 + 0.5;

	//		output_albedo[writecoord] = average;
	//		output_normal[writecoord] = average_normal;

	//		InterlockedExchange(lock[writecoord], 0, locked);
	//		done = true;
	//	}
	//}
}
