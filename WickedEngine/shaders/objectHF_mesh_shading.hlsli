#include "globals.hlsli"
#include "objectHF.hlsli"

// AS sample: https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/Samples/Desktop/D3D12MeshShaders/src/MeshletCull/MeshletAS.hlsl
// MS sample: https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/Samples/Desktop/D3D12MeshShaders/src/MeshletCull/MeshletMS.hlsl

static const uint AS_GROUPSIZE = 32;
static const uint MS_GROUPSIZE = 128;

struct AmplificationPayload
{
	uint instanceID;
	uint clusters[AS_GROUPSIZE];
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

	// Culling:
	if (visible && geometry.vb_pre < 0) // vb_pre < 0 when object is not skinned, currently skinned clusters cannot be culled
	{
		ShaderCluster cluster = bindless_structured_cluster[geometry.vb_clu][clusterID];
		ShaderCamera camera = GetCamera();
		
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
	uint primitiveID : SV_PrimitiveID;
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
#if defined(OBJECTSHADER_LAYOUT_PREPASS) || defined(OBJECTSHADER_LAYOUT_PREPASS_TEX)
	,out primitives PrimitiveAttributes primitives[CLUSTER_TRIANGLE_COUNT]
#endif // defined(OBJECTSHADER_LAYOUT_PREPASS) || defined(OBJECTSHADER_LAYOUT_PREPASS_TEX)
	)
{
	uint clusterID = amplification_payload.clusters[Gid];
	ShaderGeometry geometry = GetMesh();
	if(clusterID >= geometry.clusterOffset + geometry.clusterCount)
		return;
	ShaderCluster cluster = bindless_structured_cluster[geometry.vb_clu][clusterID];
    SetMeshOutputCounts(cluster.vertexCount(), cluster.triangleCount());
	if (groupIndex < cluster.vertexCount())
	{
		uint vertexID = cluster.vertices[groupIndex];

		VertexInput input;
		input.vertexID = cluster.vertices[groupIndex];
		input.instanceID = amplification_payload.instanceID;
		
		verts[groupIndex] = vertex_to_pixel_export(input);
	}
	for (uint i = groupIndex; i < cluster.triangleCount(); i += MS_GROUPSIZE)
	{
		triangles[i] = cluster.triangles[i].tri();
		
#if defined(OBJECTSHADER_LAYOUT_PREPASS) || defined(OBJECTSHADER_LAYOUT_PREPASS_TEX)
		primitives[groupIndex].primitiveID = i;
#endif // defined(OBJECTSHADER_LAYOUT_PREPASS) || defined(OBJECTSHADER_LAYOUT_PREPASS_TEX)
	}
}
#endif // OBJECTSHADER_COMPILE_MS
