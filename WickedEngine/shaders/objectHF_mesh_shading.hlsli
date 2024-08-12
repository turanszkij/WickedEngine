#include "globals.hlsli"
#include "objectHF.hlsli"

static const uint AS_GROUPSIZE = 64;
static const uint MS_GROUPSIZE = 64;

struct AmplificationPayload
{
	uint instanceID;
	uint clusters[AS_GROUPSIZE];
};

#ifdef OBJECTSHADER_COMPILE_AS
groupshared AmplificationPayload amplification_payload;

[numthreads(AS_GROUPSIZE, 1, 1)]
void main(uint Gid : SV_GroupID, uint DTid : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex)
{
	uint instanceID = Gid;
	uint clusterID = groupIndex;

	ShaderMeshInstancePointer poi = bindless_buffers[push.instances].Load<ShaderMeshInstancePointer>(push.instance_offset + instanceID * sizeof(ShaderMeshInstancePointer));
	ShaderMeshInstance inst = load_instance(poi.GetInstanceIndex());
	ShaderGeometry mesh = GetMesh();
	
	amplification_payload.instanceID = instanceID;
	amplification_payload.clusters[clusterID] = clusterID;

	uint count, stride;
	bindless_structured_cluster[mesh.vb_clu].GetDimensions(count, stride);
	
	DispatchMesh(count, 1, 1, amplification_payload);
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
	ShaderGeometry mesh = GetMesh();
	ShaderCluster cluster = bindless_structured_cluster[mesh.vb_clu][clusterID];
    SetMeshOutputCounts(cluster.vertex_count, cluster.triangle_count);
	if (groupIndex < cluster.vertex_count)
	{
		uint vertexID = cluster.vertices[groupIndex];

		VertexInput input;
		input.vertexID = cluster.vertices[groupIndex];
		input.instanceID = amplification_payload.instanceID;
		
		verts[groupIndex] = vertex_to_pixel_export(input);

		#ifdef OBJECTSHADER_USE_COLOR
		verts[groupIndex].color = 1;
		#endif
	}
	for (uint i = groupIndex; i < cluster.triangle_count; i += MS_GROUPSIZE)
	{
		triangles[i] = cluster.triangles[i].tri();
		
#if defined(OBJECTSHADER_LAYOUT_PREPASS) || defined(OBJECTSHADER_LAYOUT_PREPASS_TEX)
		primitives[groupIndex].primitiveID = i;
#endif // defined(OBJECTSHADER_LAYOUT_PREPASS) || defined(OBJECTSHADER_LAYOUT_PREPASS_TEX)
	}
}
#endif // OBJECTSHADER_COMPILE_MS
