#include "globals.hlsli"
#include "objectHF.hlsli"

#define FRUSTUM_CULLING
#define CONE_CULLING
#define ZERO_AREA_CULLING
#define OCCLUSION_CULLING

// Note: with current 32-bit bitmask visibility solution, exactly 32 threads are required in both stages!
static const uint AS_GROUPSIZE = 32;
static const uint MS_GROUPSIZE = 32;

struct AmplificationPayload
{
	uint instanceID;
	uint meshletGroupOffset;
	uint meshlets; // 32 meshlets bitmask
};

#ifdef OBJECTSHADER_COMPILE_AS
groupshared AmplificationPayload amplification_payload;

// Cluster LOD selection with screen space error. The allowed error per view distance and the minimum distance are the same for the whole instance,
//	so every cluster that shares the same group bounds makes the same decision, which keeps the selected cut of the cluster DAG crack-free
struct ClusterLODSelector
{
	float4x4 transform;
	float3 camera_position;
	float size;
	float threshold; // allowed mesh space error per view distance (or absolute error for ortho)
	float min_distance; // closest possible distance of any cluster of the instance
	bool ortho;

	void init(in ShaderCamera camera, in ShaderMeshInstance inst, in ShaderGeometry geometry)
	{
		transform = inst.transformRaw.GetMatrix();
		camera_position = camera.position;
		size = inst.GetSize();
		ortho = camera.IsOrtho();
		const float pixels_per_error = camera.projection[1][1] * 0.5 * camera.internal_resolution.y; // at unit distance
		// Skinned and soft body geometry deforms away from the simplified bind pose, so it always uses the full detail clusters:
		threshold = geometry.vb_pre < 0 && pixels_per_error > 0 ? GetFrame().cluster_lod_threshold / pixels_per_error : 0;
		min_distance = ortho ? 1 : max(0, distance(camera.position, inst.center) - inst.radius);
	}
	bool coarse_enough(in ShaderSphere sphere, float error)
	{
		float dist = 1;
		if (!ortho)
		{
			const float3 center = mul(transform, float4(sphere.center, 1)).xyz;
			dist = max(min_distance, distance(camera_position, center) - sphere.radius * size);
		}
		return error * size <= threshold * dist;
	}
};

[numthreads(AS_GROUPSIZE, 1, 1)]
void main(uint3 Gid : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
	ShaderGeometry geometry = GetMesh();
	
	uint instanceID = Gid.y;
	uint meshletGroupOffset = geometry.meshletOffset + Gid.x * AS_GROUPSIZE;

	if (WaveIsFirstLane())
	{
		amplification_payload.instanceID = instanceID;
		amplification_payload.meshletGroupOffset = meshletGroupOffset;
	}

	ShaderMeshInstancePointer poi = bindless_buffers[descriptor_index(push.instances)].Load<ShaderMeshInstancePointer>(push.instance_offset + instanceID * sizeof(ShaderMeshInstancePointer));
	ShaderMeshInstance inst = load_instance(poi.GetInstanceIndex());
	
	uint meshletID = meshletGroupOffset + groupIndex;

	bool visible = meshletID < geometry.meshletOffset + geometry.meshletCount;
	
	ShaderCamera camera = GetCameraIndexed(poi.GetCameraIndex());

	// Cluster LOD selection:
	[branch]
	if (geometry.vb_lod >= 0)
	{
		ClusterLODSelector selector;
		selector.init(camera, inst, geometry);
		ByteAddressBuffer lod_buffer = bindless_buffers[descriptor_index(geometry.vb_lod)];

		// Clusters of a subset are sorted by decreasing simplified error, so if the first cluster of this group is coarse enough even at the closest possible distance, all of them are:
		const float max_simplified_error = lod_buffer.Load<ShaderClusterLOD>(meshletGroupOffset * sizeof(ShaderClusterLOD)).simplified_error;
		if (max_simplified_error * selector.size <= selector.threshold * selector.min_distance)
		{
			visible = false;
		}
		else if (visible)
		{
			ShaderClusterLOD lod = lod_buffer.Load<ShaderClusterLOD>(meshletID * sizeof(ShaderClusterLOD));
			visible = !selector.coarse_enough(lod.simplified, lod.simplified_error) && selector.coarse_enough(lod.refined, lod.refined_error);
		}
	}

	// Meshlet culling:
	if (visible)
	{
		ShaderClusterBounds bounds = bindless_structured_cluster_bounds[descriptor_index(geometry.vb_bou)][meshletID];
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
				bounds.cone_axis = mul(inst.transformRaw.GetMatrixAdjoint(), bounds.cone_axis);
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
				Texture2D reprojected_depth = bindless_textures[descriptor_index(camera.texture_reprojected_depth_index)];
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

	amplification_payload.meshlets = WaveActiveBallot(visible);
	const uint visibleCount = WaveActiveCountBits(visible);
	
	DispatchMesh(visibleCount, 1, 1, amplification_payload);
}
#endif // OBJECTSHADER_COMPILE_AS

#ifdef OBJECTSHADER_COMPILE_MS

struct PrimitiveAttributes
{
#if defined(OBJECTSHADER_USE_INSTANCEINDEX) || defined(OBJECTSHADER_USE_DITHERING) || defined(OBJECTSHADER_USE_CAMERAINDEX)
	uint poi : INSTANCEPOINTER;
#endif // OBJECTSHADER_USE_INSTANCEINDEX || OBJECTSHADER_USE_DITHERING || OBJECTSHADER_USE_CAMERAINDEX

#if defined(OBJECTSHADER_LAYOUT_PREPASS) || defined(OBJECTSHADER_LAYOUT_PREPASS_TEX)
	uint primitiveID : PRIMITIVEID;
#endif // defined(OBJECTSHADER_LAYOUT_PREPASS) || defined(OBJECTSHADER_LAYOUT_PREPASS_TEX)

#ifdef OBJECTSHADER_USE_RENDERTARGETARRAYINDEX
	uint RTIndex : SV_RenderTargetArrayIndex;
#endif // OBJECTSHADER_USE_RENDERTARGETARRAYINDEX

#ifdef OBJECTSHADER_USE_VIEWPORTARRAYINDEX
	uint VPIndex : SV_ViewportArrayIndex;
#endif // OBJECTSHADER_USE_VIEWPORTARRAYINDEX
};

groupshared uint shared_groupMeshletIndex;

[outputtopology("triangle")]
[numthreads(MS_GROUPSIZE, 1, 1)]
void main(
	uint DTid : SV_DispatchThreadID,
	uint Gid : SV_GroupID,
	uint groupIndex : SV_GroupIndex,
	in payload AmplificationPayload amplification_payload,
	out indices uint3 triangles[MESHLET_TRIANGLE_COUNT],
	out vertices PixelInput verts[MESHLET_VERTEX_COUNT]
#if defined(OBJECTSHADER_LAYOUT_PREPASS) || defined(OBJECTSHADER_LAYOUT_PREPASS_TEX) || defined(OBJECTSHADER_USE_INSTANCEINDEX) || defined(OBJECTSHADER_USE_DITHERING) || defined(OBJECTSHADER_USE_RENDERTARGETARRAYINDEX) || defined(OBJECTSHADER_USE_VIEWPORTARRAYINDEX)
	,out primitives PrimitiveAttributes primitives[MESHLET_TRIANGLE_COUNT]
#endif
	)
{
	// Scan for group-relative visible meshlet bit that matches current groupID:
	uint groupMeshletIndex = 0;
	if (amplification_payload.meshlets == ~0u)
	{
		// Full visibility bitmask: no need to search, every group of mesh shader maps to every thread of amplification shader
		groupMeshletIndex = Gid;
	}
	else
	{
		// Sparse visibility bitmask: need to find the SV_GroupID-th visible bit
		const bool threadValidBit = amplification_payload.meshlets & (1u << groupIndex);
		const uint prefixActiveBits = WavePrefixCountBits(threadValidBit);
		if (threadValidBit && prefixActiveBits == Gid)
			shared_groupMeshletIndex = groupIndex;
		GroupMemoryBarrierWithGroupSync();
		groupMeshletIndex = shared_groupMeshletIndex;
	}

	const uint meshletID = amplification_payload.meshletGroupOffset + groupMeshletIndex;
	ShaderGeometry geometry = GetMesh();
	ShaderCluster cluster = bindless_structured_cluster[descriptor_index(geometry.vb_clu)][meshletID];
	SetMeshOutputCounts(cluster.vertexCount, cluster.triangleCount);

	for (uint vi = groupIndex; vi < cluster.vertexCount; vi += MS_GROUPSIZE)
	{
		uint vertexID = cluster.vertices[vi];

		VertexInput input;
		input.vertexID = vertexID;
		input.instanceID = amplification_payload.instanceID;
		
		verts[vi] = vertex_to_pixel_export(input);
	}

	ShaderMeshInstancePointer poi = bindless_buffers[descriptor_index(push.instances)].Load<ShaderMeshInstancePointer>(push.instance_offset + amplification_payload.instanceID * sizeof(ShaderMeshInstancePointer));
	ShaderMeshInstance inst = load_instance(poi.GetInstanceIndex());
	const uint frustum_index = poi.GetCameraIndex();
	
	for (uint ti = groupIndex; ti < cluster.triangleCount; ti += MS_GROUPSIZE)
	{
		triangles[ti] = cluster.triangles[ti].tri();
		
#if defined(OBJECTSHADER_USE_INSTANCEINDEX) || defined(OBJECTSHADER_USE_DITHERING) || defined(OBJECTSHADER_USE_CAMERAINDEX)
		primitives[ti].poi = poi.data;
#endif // defined(OBJECTSHADER_USE_INSTANCEINDEX) || defined(OBJECTSHADER_USE_DITHERING) || defined(OBJECTSHADER_USE_CAMERAINDEX)
		
#if defined(OBJECTSHADER_LAYOUT_PREPASS) || defined(OBJECTSHADER_LAYOUT_PREPASS_TEX)
		PrimitiveID prim;
		prim.init();
		prim.primitiveIndex = (meshletID - geometry.meshletOffset) * MESHLET_TRIANGLE_COUNT + ti;
		prim.instanceIndex = poi.GetInstanceIndex();
		prim.subsetIndex = push.geometryIndex - inst.geometryOffset;
		primitives[ti].primitiveID = prim.pack(inst, geometry);
#endif // defined(OBJECTSHADER_LAYOUT_PREPASS) || defined(OBJECTSHADER_LAYOUT_PREPASS_TEX)
		
#ifdef OBJECTSHADER_USE_RENDERTARGETARRAYINDEX
		primitives[ti].RTIndex = frustum_index;
#endif // OBJECTSHADER_USE_RENDERTARGETARRAYINDEX

#ifdef OBJECTSHADER_USE_VIEWPORTARRAYINDEX
		primitives[ti].VPIndex = frustum_index;
#endif // OBJECTSHADER_USE_VIEWPORTARRAYINDEX

	}
}
#endif // OBJECTSHADER_COMPILE_MS
