#include "globals.hlsli"
#include "objectHF.hlsli"

#define FRUSTUM_CULLING
#define CONE_CULLING

static const uint AS_GROUPSIZE = 32;
static const uint MS_GROUPSIZE = 128;

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
	if (visible && geometry.vb_pre < 0) // vb_pre < 0 when object is not skinned, currently skinned clusters cannot be culled
	{
		ShaderClusterBounds bounds = bindless_structured_cluster_bounds[geometry.vb_bou][meshletID];
		ShaderCamera camera = GetCamera(poi.GetCameraIndex());

#ifdef FRUSTUM_CULLING
		// Frustum culling:
		bounds.sphere.center = mul(inst.transformRaw.GetMatrix(), float4(bounds.sphere.center, 1)).xyz;
		bounds.sphere.radius = max3(mul((float3x3)inst.transformRaw.GetMatrix(), bounds.sphere.radius.xxx));
		visible &= camera.frustum.intersects(bounds.sphere);
#endif // FRUSTUM_CULLING

#ifdef CONE_CULLING
		if((geometry.flags & SHADERMESH_FLAG_DOUBLE_SIDED) == 0) // disable cone culling for double sided
		{
			// Cone culling:
			bounds.cone_axis = rotate_vector(bounds.cone_axis, inst.quaternion);
			if (camera.options & SHADERCAMERA_OPTION_ORTHO)
			{
				visible &= dot(camera.forward, bounds.cone_axis) < bounds.cone_cutoff;
			}
			else
			{
				visible &= dot(bounds.sphere.center - camera.position, bounds.cone_axis) < bounds.cone_cutoff * length(bounds.sphere.center - camera.position) + bounds.sphere.radius;
			}
		}
#endif // CONE_CULLING
		
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
#if defined(OBJECTSHADER_LAYOUT_PREPASS) || defined(OBJECTSHADER_LAYOUT_PREPASS_TEX)
	uint primitiveID : SV_PrimitiveID;
#endif // defined(OBJECTSHADER_LAYOUT_PREPASS) || defined(OBJECTSHADER_LAYOUT_PREPASS_TEX)
	
#ifdef OBJECTSHADER_USE_RENDERTARGETARRAYINDEX
	uint RTIndex : SV_RenderTargetArrayIndex;
#endif // OBJECTSHADER_USE_RENDERTARGETARRAYINDEX

#ifdef OBJECTSHADER_USE_VIEWPORTARRAYINDEX
	uint VPIndex : SV_ViewportArrayIndex;
#endif // OBJECTSHADER_USE_VIEWPORTARRAYINDEX
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
	if(meshletID >= geometry.meshletOffset + geometry.meshletCount)
		return;
	ShaderCluster cluster = bindless_structured_cluster[geometry.vb_clu][meshletID];
	ShaderMeshInstancePointer poi = bindless_buffers[push.instances].Load<ShaderMeshInstancePointer>(push.instance_offset + amplification_payload.instanceID * sizeof(ShaderMeshInstancePointer));
	SetMeshOutputCounts(cluster.vertexCount, cluster.triangleCount);
	const uint vi = groupIndex;
	if (vi < cluster.vertexCount)
	{
		uint vertexID = cluster.vertices[vi];

		VertexInput input;
		input.vertexID = cluster.vertices[vi];
		input.instanceID = amplification_payload.instanceID;
		
		verts[vi] = vertex_to_pixel_export(input);
	}
	const uint ti = MESHLET_TRIANGLE_COUNT - 1 - groupIndex; // reverse the order of threads, so those which are not writing vertices are utilized earlier
	if (ti < cluster.triangleCount)
	{
		triangles[ti] = cluster.triangles[ti].tri();
		
#if defined(OBJECTSHADER_LAYOUT_PREPASS) || defined(OBJECTSHADER_LAYOUT_PREPASS_TEX)
		primitives[ti].primitiveID = (meshletID - geometry.meshletOffset) * MESHLET_TRIANGLE_COUNT + ti;
#endif // defined(OBJECTSHADER_LAYOUT_PREPASS) || defined(OBJECTSHADER_LAYOUT_PREPASS_TEX)

#ifdef OBJECTSHADER_USE_RENDERTARGETARRAYINDEX
		const uint frustum_index = poi.GetCameraIndex();
		primitives[ti].RTIndex = GetCamera(frustum_index).output_index;
#endif // OBJECTSHADER_USE_RENDERTARGETARRAYINDEX

#ifdef OBJECTSHADER_USE_VIEWPORTARRAYINDEX
		const uint frustum_index = poi.GetCameraIndex();
		primitives[ti].VPIndex = GetCamera(frustum_index).output_index;
#endif // OBJECTSHADER_USE_VIEWPORTARRAYINDEX
	}
}
#endif // OBJECTSHADER_COMPILE_MS
