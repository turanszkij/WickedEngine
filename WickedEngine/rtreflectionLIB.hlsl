#define DISABLE_SOFT_SHADOWMAP
#define DISABLE_TRANSPARENT_SHADOWMAP

#ifndef SPIRV
// Vulkan shader compiler has problem with this
//	https://github.com/microsoft/DirectXShaderCompiler/issues/3119
#define RAYTRACING_INLINE
#endif // SPIRV

#include "globals.hlsli"

RAYTRACINGACCELERATIONSTRUCTURE(scene_acceleration_structure, TEXSLOT_ACCELERATION_STRUCTURE);

#include "ShaderInterop_Postprocess.h"
#include "raytracingHF.hlsli"
#include "stochasticSSRHF.hlsli"
#include "lightingHF.hlsli"

RWTEXTURE2D(output, float4, 0);

ConstantBuffer<ShaderMaterial> subsets_material[] : register(b0, space1);
Texture2D<float4> subsets_textures[] : register(t0, space2);
Buffer<uint> subsets_indexBuffer[] : register(t0, space3);
ByteAddressBuffer subsets_vertexBuffer_RAW[] : register(t0, space4);
Buffer<float2> subsets_vertexBuffer_UVSETS[] : register(t0, space5);

struct RayPayload
{
	float3 color;
	float roughness;
};

[shader("raygeneration")]
void RTReflection_Raygen()
{
	uint2 DTid = DispatchRaysIndex().xy;
	const float2 uv = ((float2)DTid.xy + 0.5) / (float2)DispatchRaysDimensions();
	const float depth = texture_depth.SampleLevel(sampler_point_clamp, uv, 1);
	if (depth == 0)
		return;

	const float2 velocity = texture_gbuffer2.SampleLevel(sampler_point_clamp, uv, 0).xy;
	const float2 prevUV = uv + velocity;

	const float4 g1 = texture_gbuffer1.SampleLevel(sampler_linear_clamp, prevUV, 0);
	const float3 P = reconstructPosition(uv, depth);
	const float3 N = normalize(g1.rgb * 2 - 1);
	const float3 V = normalize(g_xCamera_CamPos - P);
	const float roughness = g1.a;


	// The ray direction selection part is the same as in from ssr_raytraceCS.hlsl:
	float4 H;
	float3 L;
	if (roughness > 0.05f)
	{
		float3x3 tangentBasis = GetTangentBasis(N);
		float3 tangentV = mul(tangentBasis, V);


		float2 Xi;
		Xi.x = BNDSequenceSample(DTid.xy, g_xFrame_FrameCount, 0);
		Xi.y = BNDSequenceSample(DTid.xy, g_xFrame_FrameCount, 1);

		Xi.y = lerp(Xi.y, 0.0f, GGX_IMPORTANCE_SAMPLE_BIAS);

		H = ImportanceSampleVisibleGGX(SampleDisk(Xi), roughness, tangentV);

		// Tangent to world
		H.xyz = mul(H.xyz, tangentBasis);


		L = reflect(-V, H.xyz);
	}
	else
	{
		H = float4(N.xyz, 1.0f);
		L = reflect(-V, H.xyz);
	}


	const float3 R = L;

	float seed = g_xFrame_Time;

	RayDesc ray;
	ray.TMin = 0.05;
	ray.TMax = rtreflection_range;
	ray.Origin = trace_bias_position(P, N);
	ray.Direction = normalize(R);

	RayPayload payload;
	payload.color = 0;
	payload.roughness = roughness;

	TraceRay(
		scene_acceleration_structure,   // AccelerationStructure
		0,                              // RayFlags
		~0,                             // InstanceInclusionMask
		0,                              // RayContributionToHitGroupIndex
		0,                              // MultiplierForGeomtryContributionToShaderIndex
		0,                              // MissShaderIndex
		ray,                            // Ray
		payload                         // Payload
	);

	output[DTid.xy] = float4(payload.color, 1);
}

[shader("closesthit")]
void RTReflection_ClosestHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
	float u = attr.barycentrics.x;
	float v = attr.barycentrics.y;
	float w = 1 - u - v;
	uint primitiveIndex = PrimitiveIndex();
	uint geometryOffset = InstanceID();
	uint geometryIndex = GeometryIndex(); // requires tier_1_1 GeometryIndex feature!!
	uint descriptorIndex = geometryOffset + geometryIndex;
	ShaderMaterial material = subsets_material[descriptorIndex];
	uint i0 = subsets_indexBuffer[descriptorIndex][primitiveIndex * 3 + 0];
	uint i1 = subsets_indexBuffer[descriptorIndex][primitiveIndex * 3 + 1];
	uint i2 = subsets_indexBuffer[descriptorIndex][primitiveIndex * 3 + 2];
	float4 uv0, uv1, uv2;
	uv0.xy = subsets_vertexBuffer_UVSETS[descriptorIndex * VERTEXBUFFER_DESCRIPTOR_UV_COUNT + VERTEXBUFFER_DESCRIPTOR_UV_0][i0];
	uv1.xy = subsets_vertexBuffer_UVSETS[descriptorIndex * VERTEXBUFFER_DESCRIPTOR_UV_COUNT + VERTEXBUFFER_DESCRIPTOR_UV_0][i1];
	uv2.xy = subsets_vertexBuffer_UVSETS[descriptorIndex * VERTEXBUFFER_DESCRIPTOR_UV_COUNT + VERTEXBUFFER_DESCRIPTOR_UV_0][i2];
	uv0.zw = subsets_vertexBuffer_UVSETS[descriptorIndex * VERTEXBUFFER_DESCRIPTOR_UV_COUNT + VERTEXBUFFER_DESCRIPTOR_UV_1][i0];
	uv1.zw = subsets_vertexBuffer_UVSETS[descriptorIndex * VERTEXBUFFER_DESCRIPTOR_UV_COUNT + VERTEXBUFFER_DESCRIPTOR_UV_1][i1];
	uv2.zw = subsets_vertexBuffer_UVSETS[descriptorIndex * VERTEXBUFFER_DESCRIPTOR_UV_COUNT + VERTEXBUFFER_DESCRIPTOR_UV_1][i2];
	float3 n0, n1, n2;
	const uint stride_POS = 16;
	n0 = unpack_unitvector(subsets_vertexBuffer_RAW[descriptorIndex * VERTEXBUFFER_DESCRIPTOR_RAW_COUNT + VERTEXBUFFER_DESCRIPTOR_RAW_POS].Load4(i0 * stride_POS).w);
	n1 = unpack_unitvector(subsets_vertexBuffer_RAW[descriptorIndex * VERTEXBUFFER_DESCRIPTOR_RAW_COUNT + VERTEXBUFFER_DESCRIPTOR_RAW_POS].Load4(i1 * stride_POS).w);
	n2 = unpack_unitvector(subsets_vertexBuffer_RAW[descriptorIndex * VERTEXBUFFER_DESCRIPTOR_RAW_COUNT + VERTEXBUFFER_DESCRIPTOR_RAW_POS].Load4(i2 * stride_POS).w);

	float4 uvsets = uv0 * w + uv1 * u + uv2 * v;
	float3 N = n0 * w + n1 * u + n2 * v;

	N = mul((float3x3)ObjectToWorld3x4(), N);
	N = normalize(N);

	float4 baseColor = material.baseColor;
	[branch]
	if (material.uvset_baseColorMap >= 0 && (g_xFrame_Options & OPTION_BIT_DISABLE_ALBEDO_MAPS) == 0)
	{
		const float2 UV_baseColorMap = material.uvset_baseColorMap == 0 ? uvsets.xy : uvsets.zw;
		baseColor = subsets_textures[descriptorIndex * MATERIAL_TEXTURE_SLOT_DESCRIPTOR_COUNT + MATERIAL_TEXTURE_SLOT_DESCRIPTOR_BASECOLOR].SampleLevel(sampler_linear_wrap, UV_baseColorMap, 2);
		baseColor.rgb *= DEGAMMA(baseColor.rgb);
	}

	[branch]
	if (material.IsUsingVertexColors())
	{
		float4 c0, c1, c2;
		const uint stride_COL = 4;
		c0 = unpack_rgba(subsets_vertexBuffer_RAW[descriptorIndex * VERTEXBUFFER_DESCRIPTOR_RAW_COUNT + VERTEXBUFFER_DESCRIPTOR_RAW_COL].Load(i0 * stride_COL));
		c1 = unpack_rgba(subsets_vertexBuffer_RAW[descriptorIndex * VERTEXBUFFER_DESCRIPTOR_RAW_COUNT + VERTEXBUFFER_DESCRIPTOR_RAW_COL].Load(i1 * stride_COL));
		c2 = unpack_rgba(subsets_vertexBuffer_RAW[descriptorIndex * VERTEXBUFFER_DESCRIPTOR_RAW_COUNT + VERTEXBUFFER_DESCRIPTOR_RAW_COL].Load(i2 * stride_COL));
		float4 vertexColor = c0 * w + c1 * u + c2 * v;
		baseColor *= vertexColor;
	}

	[branch]
	if (material.normalMapStrength > 0 && material.uvset_normalMap >= 0)
	{
		float4 t0, t1, t2;
		const uint stride_TAN = 4;
		t0 = unpack_utangent(subsets_vertexBuffer_RAW[descriptorIndex * VERTEXBUFFER_DESCRIPTOR_RAW_COUNT + VERTEXBUFFER_DESCRIPTOR_RAW_TAN].Load(i0 * stride_TAN));
		t1 = unpack_utangent(subsets_vertexBuffer_RAW[descriptorIndex * VERTEXBUFFER_DESCRIPTOR_RAW_COUNT + VERTEXBUFFER_DESCRIPTOR_RAW_TAN].Load(i1 * stride_TAN));
		t2 = unpack_utangent(subsets_vertexBuffer_RAW[descriptorIndex * VERTEXBUFFER_DESCRIPTOR_RAW_COUNT + VERTEXBUFFER_DESCRIPTOR_RAW_TAN].Load(i2 * stride_TAN));
		float4 T = t0 * w + t1 * u + t2 * v;
		T = T * 2 - 1;
		T.xyz = mul((float3x3)ObjectToWorld3x4(), T.xyz);
		T.xyz = normalize(T.xyz);
		float3 B = normalize(cross(T.xyz, N) * T.w);
		float3x3 TBN = float3x3(T.xyz, B, N);

		const float2 UV_normalMap = material.uvset_normalMap == 0 ? uvsets.xy : uvsets.zw;
		float3 normalMap = subsets_textures[descriptorIndex * MATERIAL_TEXTURE_SLOT_DESCRIPTOR_COUNT + MATERIAL_TEXTURE_SLOT_DESCRIPTOR_NORMAL].SampleLevel(sampler_linear_wrap, UV_normalMap, 2).rgb;
		normalMap = normalMap * 2 - 1;
		N = normalize(lerp(N, mul(normalMap, TBN), material.normalMapStrength));
	}

	float4 surfaceMap = 1;
	[branch]
	if (material.uvset_surfaceMap >= 0)
	{
		const float2 UV_surfaceMap = material.uvset_surfaceMap == 0 ? uvsets.xy : uvsets.zw;
		surfaceMap = subsets_textures[descriptorIndex * MATERIAL_TEXTURE_SLOT_DESCRIPTOR_COUNT + MATERIAL_TEXTURE_SLOT_DESCRIPTOR_SURFACE].SampleLevel(sampler_linear_wrap, UV_surfaceMap, 2);
	}

	Surface surface;
	surface.create(material, baseColor, surfaceMap);

	surface.pixel = DispatchRaysIndex().xy;
	surface.screenUV = surface.pixel / (float2)DispatchRaysDimensions().xy;

	[branch]
	if (material.IsOcclusionEnabled_Secondary() && material.uvset_occlusionMap >= 0)
	{
		const float2 UV_occlusionMap = material.uvset_occlusionMap == 0 ? uvsets.xy : uvsets.zw;
		surface.occlusion *= subsets_textures[descriptorIndex * MATERIAL_TEXTURE_SLOT_DESCRIPTOR_COUNT + MATERIAL_TEXTURE_SLOT_DESCRIPTOR_OCCLUSION].SampleLevel(sampler_linear_wrap, UV_occlusionMap, 2).r;
	}

	surface.emissiveColor = material.emissiveColor;
	[branch]
	if (material.uvset_emissiveMap >= 0)
	{
		const float2 UV_emissiveMap = material.uvset_emissiveMap == 0 ? uvsets.xy : uvsets.zw;
		float4 emissiveMap = subsets_textures[descriptorIndex * MATERIAL_TEXTURE_SLOT_DESCRIPTOR_COUNT + MATERIAL_TEXTURE_SLOT_DESCRIPTOR_EMISSIVE].SampleLevel(sampler_linear_wrap, UV_emissiveMap, 2);
		emissiveMap.rgb = DEGAMMA(emissiveMap.rgb);
		surface.emissiveColor *= emissiveMap;
	}


	// Light sampling:
	surface.P = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
	surface.V = -WorldRayDirection();
	surface.N = N;
	surface.update();

	Lighting lighting;
	lighting.create(0, 0, GetAmbient(surface.N), 0);

	[loop]
	for (uint iterator = 0; iterator < g_xFrame_LightArrayCount; iterator++)
	{
		ShaderEntity light = EntityArray[g_xFrame_LightArrayOffset + iterator];

		if (light.GetFlags() & ENTITY_FLAG_LIGHT_STATIC)
		{
			continue; // static lights will be skipped (they are used in lightmap baking)
		}

		switch (light.GetType())
		{
		case ENTITY_TYPE_DIRECTIONALLIGHT:
		{
			DirectionalLight(light, surface, lighting);
		}
		break;
		case ENTITY_TYPE_POINTLIGHT:
		{
			PointLight(light, surface, lighting);
		}
		break;
		case ENTITY_TYPE_SPOTLIGHT:
		{
			SpotLight(light, surface, lighting);
		}
		break;
		}
	}

	LightingPart combined_lighting = CombineLighting(surface, lighting);
	payload.color = surface.albedo * combined_lighting.diffuse + combined_lighting.specular + surface.emissiveColor.rgb * surface.emissiveColor.a;

}

[shader("anyhit")]
void RTReflection_AnyHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
	float u = attr.barycentrics.x;
	float v = attr.barycentrics.y;
	float w = 1 - u - v;
	uint primitiveIndex = PrimitiveIndex();
	uint geometryOffset = InstanceID();
	uint geometryIndex = GeometryIndex(); // requires tier_1_1 GeometryIndex feature!!
	uint descriptorIndex = geometryOffset + geometryIndex;
	ShaderMaterial material = subsets_material[descriptorIndex];
	if (material.uvset_baseColorMap < 0)
	{
		return;
	}
	uint i0 = subsets_indexBuffer[descriptorIndex][primitiveIndex * 3 + 0];
	uint i1 = subsets_indexBuffer[descriptorIndex][primitiveIndex * 3 + 1];
	uint i2 = subsets_indexBuffer[descriptorIndex][primitiveIndex * 3 + 2];
	float2 uv0, uv1, uv2;
	if (material.uvset_baseColorMap == 0)
	{
		uv0 = subsets_vertexBuffer_UVSETS[descriptorIndex * VERTEXBUFFER_DESCRIPTOR_UV_COUNT + VERTEXBUFFER_DESCRIPTOR_UV_0][i0];
		uv1 = subsets_vertexBuffer_UVSETS[descriptorIndex * VERTEXBUFFER_DESCRIPTOR_UV_COUNT + VERTEXBUFFER_DESCRIPTOR_UV_0][i1];
		uv2 = subsets_vertexBuffer_UVSETS[descriptorIndex * VERTEXBUFFER_DESCRIPTOR_UV_COUNT + VERTEXBUFFER_DESCRIPTOR_UV_0][i2];
	}
	else
	{
		uv0 = subsets_vertexBuffer_UVSETS[descriptorIndex * VERTEXBUFFER_DESCRIPTOR_UV_COUNT + VERTEXBUFFER_DESCRIPTOR_UV_1][i0];
		uv1 = subsets_vertexBuffer_UVSETS[descriptorIndex * VERTEXBUFFER_DESCRIPTOR_UV_COUNT + VERTEXBUFFER_DESCRIPTOR_UV_1][i1];
		uv2 = subsets_vertexBuffer_UVSETS[descriptorIndex * VERTEXBUFFER_DESCRIPTOR_UV_COUNT + VERTEXBUFFER_DESCRIPTOR_UV_1][i2];
	}

	float2 uv = uv0 * w + uv1 * u + uv2 * v;
	float alpha = subsets_textures[descriptorIndex * MATERIAL_TEXTURE_SLOT_DESCRIPTOR_COUNT + MATERIAL_TEXTURE_SLOT_DESCRIPTOR_BASECOLOR].SampleLevel(sampler_point_wrap, uv, 2).a;

	if (alpha - material.alphaTest < 0)
	{
		IgnoreHit();
	}
}

[shader("miss")]
void RTReflection_Miss(inout RayPayload payload)
{
	payload.color = GetDynamicSkyColor(WorldRayDirection());
}
