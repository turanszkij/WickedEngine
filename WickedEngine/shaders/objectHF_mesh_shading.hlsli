#include "globals.hlsli"
#include "objectHF.hlsli"

// AS sample: https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/Samples/Desktop/D3D12MeshShaders/src/MeshletCull/MeshletAS.hlsl
// MS sample: https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/Samples/Desktop/D3D12MeshShaders/src/MeshletCull/MeshletMS.hlsl

static const uint AS_GROUPSIZE = 32;
static const uint MS_GROUPSIZE = 64;

struct AmplificationPayload
{
	uint instanceID;
	uint16_t clusters[AS_GROUPSIZE];
};

#ifdef OBJECTSHADER_COMPILE_AS
groupshared AmplificationPayload amplification_payload;

[numthreads(AS_GROUPSIZE, 1, 1)]
void main(uint3 Gid : SV_GroupID, uint3 DTid : SV_DispatchThreadID)
{
	ShaderGeometry geometry = GetMesh();
	
	uint instanceID = Gid.y;
	amplification_payload.instanceID = instanceID;

	ShaderMeshInstancePointer poi = bindless_buffers[push.instances].Load<ShaderMeshInstancePointer>(push.instance_offset + instanceID * sizeof(ShaderMeshInstancePointer));
	ShaderMeshInstance inst = load_instance(poi.GetInstanceIndex());
	
	uint clusterID = geometry.clusterOffset + DTid.x;

	bool visible = DTid.x < geometry.clusterCount;

#if 1
	// Culling:
	if (visible && geometry.vb_pre < 0) // vb_pre < 0 when object is not skinned, currently skinned clusters cannot be culled
	{
		ShaderCluster cluster = bindless_structured_cluster[geometry.vb_clu][clusterID];
		ShaderCamera camera = GetCamera(poi.GetCameraIndex());
		
		// Frustum culling:
		cluster.sphere.center = mul(inst.transformRaw.GetMatrix(), float4(cluster.sphere.center, 1)).xyz;
		cluster.sphere.radius = max3(mul((float3x3)inst.transformRaw.GetMatrix(), cluster.sphere.radius.xxx));
		visible = camera.frustum.intersects(cluster.sphere);

		if (visible)
		{
			// Cone culling:
			cluster.cone_apex = mul(inst.transformRaw.GetMatrix(), float4(cluster.cone_apex, 1)).xyz;
			cluster.cone_axis = rotate_vector(cluster.cone_axis, inst.quaternion);
			visible = dot(normalize(cluster.cone_apex - camera.position), cluster.cone_axis) < cluster.cone_cutoff;
		}
	}
#endif

	if (visible)
	{
		uint index = WavePrefixCountBits(visible);
		amplification_payload.clusters[index] = clusterID;
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
	out vertices PixelInput verts[CLUSTER_VERTEX_COUNT],
	out indices uint3 triangles[CLUSTER_TRIANGLE_COUNT]
#if defined(OBJECTSHADER_LAYOUT_PREPASS) || defined(OBJECTSHADER_LAYOUT_PREPASS_TEX) || defined(OBJECTSHADER_USE_RENDERTARGETARRAYINDEX) || defined(OBJECTSHADER_USE_VIEWPORTARRAYINDEX)
	,out primitives PrimitiveAttributes primitives[CLUSTER_TRIANGLE_COUNT]
#endif
	)
{
	uint clusterID = amplification_payload.clusters[Gid];
	ShaderGeometry geometry = GetMesh();
	if(clusterID >= geometry.clusterOffset + geometry.clusterCount)
		return;
	ShaderCluster cluster = bindless_structured_cluster[geometry.vb_clu][clusterID];
	ShaderMeshInstancePointer poi = bindless_buffers[push.instances].Load<ShaderMeshInstancePointer>(push.instance_offset + amplification_payload.instanceID * sizeof(ShaderMeshInstancePointer));
    SetMeshOutputCounts(cluster.vertexCount(), cluster.triangleCount());
	for (uint i = groupIndex; i < cluster.vertexCount(); i += MS_GROUPSIZE)
	{
		uint vertexID = cluster.vertices[i];

		VertexInput input;
		input.vertexID = cluster.vertices[i];
		input.instanceID = amplification_payload.instanceID;
		
		verts[i] = vertex_to_pixel_export(input);
	}
	for (uint i = groupIndex; i < cluster.triangleCount(); i += MS_GROUPSIZE)
	{
		triangles[i] = cluster.triangles[i].tri();
		
#if defined(OBJECTSHADER_LAYOUT_PREPASS) || defined(OBJECTSHADER_LAYOUT_PREPASS_TEX)
		primitives[i].primitiveID = i;
#endif // defined(OBJECTSHADER_LAYOUT_PREPASS) || defined(OBJECTSHADER_LAYOUT_PREPASS_TEX)

#ifdef OBJECTSHADER_USE_RENDERTARGETARRAYINDEX
		const uint frustum_index = poi.GetCameraIndex();
		primitives[i].RTIndex = GetCamera(frustum_index).output_index;
#endif // OBJECTSHADER_USE_RENDERTARGETARRAYINDEX

#ifdef OBJECTSHADER_USE_VIEWPORTARRAYINDEX
		const uint frustum_index = poi.GetCameraIndex();
		primitives[i].VPIndex = GetCamera(frustum_index).output_index;
#endif // OBJECTSHADER_USE_VIEWPORTARRAYINDEX
	}
}
#endif // OBJECTSHADER_COMPILE_MS
