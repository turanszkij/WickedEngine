#include "globals.hlsli"
#include "objectHF.hlsli"

#define FRUSTUM_CULLING
#define CONE_CULLING
#define ZERO_AREA_CULLING
#define OCCLUSION_CULLING

static const uint AS_GROUPSIZE = 32;
static const uint MS_GROUPSIZE = 32;

struct AmplificationPayload
{
	uint instanceID;
	uint meshletGroupOffset;
	min16uint meshlets[AS_GROUPSIZE]; // this could be uint8_t if HLSL supported it because they are relative to meshletGroupOffset, maybe in the future it will be a reality
};

#ifdef OBJECTSHADER_COMPILE_AS
groupshared AmplificationPayload amplification_payload;

[numthreads(AS_GROUPSIZE, 1, 1)]
void main(uint3 Gid : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
	ShaderGeometry geometry = GetMesh();
	
	uint instanceID = Gid.y;
	uint meshletGroupOffset = geometry.meshletOffset + Gid.x * AS_GROUPSIZE;

	if(WaveIsFirstLane())
	{
		amplification_payload.instanceID = instanceID;
		amplification_payload.meshletGroupOffset = meshletGroupOffset;
	}

	ShaderMeshInstancePointer poi = bindless_buffers[push.instances].Load<ShaderMeshInstancePointer>(push.instance_offset + instanceID * sizeof(ShaderMeshInstancePointer));
	ShaderMeshInstance inst = load_instance(poi.GetInstanceIndex());
	
	uint meshletID = meshletGroupOffset + groupIndex;

	bool visible = meshletID < geometry.meshletOffset + geometry.meshletCount;
	
	// Meshlet culling:
	if (visible)
	{
		ShaderCamera camera = GetCamera(poi.GetCameraIndex());
		ShaderClusterBounds bounds = bindless_structured_cluster_bounds[geometry.vb_bou][meshletID];
		if (geometry.vb_pre >= 0)
		{
			// when object is skinned, we take the whole instance bounds, because skinned meshlet transforms are not computed
			bounds.sphere.center = inst.center;
			bounds.sphere.radius = inst.radius;
		}
		else
		{
			bounds.sphere.center = mul(inst.transformRaw.GetMatrix(), float4(bounds.sphere.center, 1)).xyz;
			bounds.sphere.radius *= inst.GetSize();
		}

		// Only allow culling when camera is not inside:
		if (distance(camera.position, bounds.sphere.center) > bounds.sphere.radius)
		{
		
#ifdef CONE_CULLING
			if((geometry.flags & SHADERMESH_FLAG_DOUBLE_SIDED) == 0 && geometry.vb_pre < 0) // disable cone culling for double sided and skinned
			{
				// Cone culling:
				bounds.cone_axis = mul(inst.transformNormal.GetMatrix(), bounds.cone_axis);
				if (camera.IsOrtho())
				{
					visible &= dot(camera.forward, bounds.cone_axis) < bounds.cone_cutoff;
				}
				else
				{
					visible &= dot(bounds.sphere.center - camera.position, bounds.cone_axis) < bounds.cone_cutoff * length(bounds.sphere.center - camera.position) + bounds.sphere.radius;
				}
			}
#endif // CONE_CULLING

#ifdef FRUSTUM_CULLING
			if (visible)
			{
				// Frustum culling:
				visible &= camera.frustum.intersects(bounds.sphere);
			}
#endif // FRUSTUM_CULLING

			if (visible && camera.texture_reprojected_depth_index >= 0)
			{
				Texture2D reprojected_depth = bindless_textures[camera.texture_reprojected_depth_index];
				float cam_sphere_distance = length(bounds.sphere.center - camera.position);
				float radius = cam_sphere_distance * tan(asin(bounds.sphere.radius / cam_sphere_distance)); // perspective distortion https://www.nickdarnell.com/hierarchical-z-buffer-occlusion-culling/
				float3 up_radius = camera.up * radius;
				float3 right = cross(camera.forward, camera.up);
				float3 right_radius = right * radius;

				float3 corner_0_world = bounds.sphere.center + up_radius - right_radius;
				float3 corner_1_world = bounds.sphere.center + up_radius + right_radius;
				float3 corner_2_world = bounds.sphere.center - up_radius - right_radius;
				float3 corner_3_world = bounds.sphere.center - up_radius + right_radius;

				float4 corner_0_clip = mul(camera.previous_view_projection, float4(corner_0_world, 1));
				float4 corner_1_clip = mul(camera.previous_view_projection, float4(corner_1_world, 1));
				float4 corner_2_clip = mul(camera.previous_view_projection, float4(corner_2_world, 1));
				float4 corner_3_clip = mul(camera.previous_view_projection, float4(corner_3_world, 1));

				float2 corner_0_uv = clipspace_to_uv(corner_0_clip.xy / corner_0_clip.w);
				float2 corner_1_uv = clipspace_to_uv(corner_1_clip.xy / corner_1_clip.w);
				float2 corner_2_uv = clipspace_to_uv(corner_2_clip.xy / corner_2_clip.w);
				float2 corner_3_uv = clipspace_to_uv(corner_3_clip.xy / corner_3_clip.w);

				float sphere_width_uv = length(corner_0_uv - corner_1_uv);

				float3 sphere_center_view = mul(camera.previous_view, float4(bounds.sphere.center, 1)).xyz;
				float3 pv = sphere_center_view - normalize(sphere_center_view) * bounds.sphere.radius;
				float4 closest_sphere_point = mul(camera.previous_projection, float4(pv, 1));

				float w = sphere_width_uv * max(camera.internal_resolution.x, camera.internal_resolution.y);

#ifdef ZERO_AREA_CULLING
				if (w < 1)
					visible = false;
#endif // ZERO_AREA_CULLING

#ifdef OCCLUSION_CULLING
				if (visible && closest_sphere_point.w > 0)
				{
					float lod = ceil(log2(w));
					float4 depths = float4(
						reprojected_depth.SampleLevel(sampler_point_clamp, corner_0_uv, lod).r,
						reprojected_depth.SampleLevel(sampler_point_clamp, corner_1_uv, lod).r,
						reprojected_depth.SampleLevel(sampler_point_clamp, corner_2_uv, lod).r,
						reprojected_depth.SampleLevel(sampler_point_clamp, corner_3_uv, lod).r
					);
					float min_depth = min(depths.x, min(depths.y, min(depths.z, depths.w)));
					float sphere_depth = closest_sphere_point.z / closest_sphere_point.w;
					if (sphere_depth < min_depth - 0.01) // little safety bias
						visible = false;
				}
#endif // OCCLUSION_CULLING
			}
		
		}
	}

	if (visible)
	{
		uint index = WavePrefixCountBits(visible);
		amplification_payload.meshlets[index] = meshletID - meshletGroupOffset; // relative packed
	}

	uint visibleCount = WaveActiveCountBits(visible);
	
	DispatchMesh(visibleCount, 1, 1, amplification_payload);
}
#endif // OBJECTSHADER_COMPILE_AS

#ifdef OBJECTSHADER_COMPILE_MS

struct PrimitiveAttributes
{
#ifdef OBJECTSHADER_USE_RENDERTARGETARRAYINDEX
	uint RTIndex : SV_RenderTargetArrayIndex;
#endif // OBJECTSHADER_USE_RENDERTARGETARRAYINDEX

#ifdef OBJECTSHADER_USE_VIEWPORTARRAYINDEX
	uint VPIndex : SV_ViewportArrayIndex;
#endif // OBJECTSHADER_USE_VIEWPORTARRAYINDEX

#if defined(OBJECTSHADER_LAYOUT_PREPASS) || defined(OBJECTSHADER_LAYOUT_PREPASS_TEX)
	uint primitiveID : SV_PrimitiveID;
#endif // defined(OBJECTSHADER_LAYOUT_PREPASS) || defined(OBJECTSHADER_LAYOUT_PREPASS_TEX)
};

[outputtopology("triangle")]
[numthreads(MS_GROUPSIZE, 1, 1)]
void main(
	uint DTid : SV_DispatchThreadID,
	uint Gid : SV_GroupID,
	uint groupIndex : SV_GroupIndex,
    in payload AmplificationPayload amplification_payload,
	out vertices PixelInput verts[MESHLET_VERTEX_COUNT],
	out indices uint3 triangles[MESHLET_TRIANGLE_COUNT]
#if defined(OBJECTSHADER_LAYOUT_PREPASS) || defined(OBJECTSHADER_LAYOUT_PREPASS_TEX) || defined(OBJECTSHADER_USE_RENDERTARGETARRAYINDEX) || defined(OBJECTSHADER_USE_VIEWPORTARRAYINDEX)
	,out primitives PrimitiveAttributes primitives[MESHLET_TRIANGLE_COUNT]
#endif
	)
{
	uint meshletGroupOffset = amplification_payload.meshletGroupOffset;
	uint meshletID = meshletGroupOffset + amplification_payload.meshlets[Gid];
	ShaderGeometry geometry = GetMesh();
	ShaderCluster cluster = bindless_structured_cluster[geometry.vb_clu][meshletID];
	SetMeshOutputCounts(cluster.vertexCount, cluster.triangleCount);

	for (uint vi = groupIndex; vi < cluster.vertexCount; vi += MS_GROUPSIZE)
	{
		uint vertexID = cluster.vertices[vi];

		VertexInput input;
		input.vertexID = cluster.vertices[vi];
		input.instanceID = amplification_payload.instanceID;
		
		verts[vi] = vertex_to_pixel_export(input);
	}

	ShaderMeshInstancePointer poi = bindless_buffers[push.instances].Load<ShaderMeshInstancePointer>(push.instance_offset + amplification_payload.instanceID * sizeof(ShaderMeshInstancePointer));
	const uint frustum_index = poi.GetCameraIndex();
	
	for (uint ti = groupIndex; ti < cluster.triangleCount; ti += MS_GROUPSIZE)
	{
		triangles[ti] = cluster.triangles[ti].tri();
		
#ifdef OBJECTSHADER_USE_RENDERTARGETARRAYINDEX
		primitives[ti].RTIndex = GetCamera(frustum_index).output_index;
#endif // OBJECTSHADER_USE_RENDERTARGETARRAYINDEX

#ifdef OBJECTSHADER_USE_VIEWPORTARRAYINDEX
		primitives[ti].VPIndex = GetCamera(frustum_index).output_index;
#endif // OBJECTSHADER_USE_VIEWPORTARRAYINDEX
		
#if defined(OBJECTSHADER_LAYOUT_PREPASS) || defined(OBJECTSHADER_LAYOUT_PREPASS_TEX)
		primitives[ti].primitiveID = (meshletID - geometry.meshletOffset) * MESHLET_TRIANGLE_COUNT + ti;
#endif // defined(OBJECTSHADER_LAYOUT_PREPASS) || defined(OBJECTSHADER_LAYOUT_PREPASS_TEX)

	}
}
#endif // OBJECTSHADER_COMPILE_MS
