#include "globals.hlsli"

static const uint THREADCOUNT = 64;

struct PushContantsMeshletPrepare
{
	uint startInstance;
};
PUSHCONSTANT(push, PushContantsMeshletPrepare);

RWStructuredBuffer<ShaderMeshInstance> output_instances : register(u0);
RWStructuredBuffer<ShaderMeshlet> output_meshlets : register(u1);
RWStructuredBuffer<uint> counterBuffer : register(u2);

groupshared uint globalMeshletOffset;

[numthreads(1, THREADCOUNT, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 Gid : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
	const uint instanceIndex = push.startInstance + Gid.x;
	ShaderMeshInstance inst = output_instances[instanceIndex];
	if (!GetCamera().frustum.intersects(inst.GetSphere()))
		return;

	if (groupIndex == 0)
	{
		InterlockedAdd(counterBuffer[0], inst.meshletCount, globalMeshletOffset);
		output_instances[instanceIndex].meshletOffset = globalMeshletOffset;
	}
	GroupMemoryBarrierWithGroupSync();
	
	ShaderGeometry mesh = load_geometry(inst.baseGeometryOffset);

	for (uint i = 0; i < inst.baseGeometryCount; ++i)
	{
		uint geometryIndex = inst.baseGeometryOffset + i;
		ShaderGeometry geometry = load_geometry(geometryIndex);
		ShaderMaterial material = load_material(geometry.materialIndex);
		const uint materialIndex_shaderType = (geometry.materialIndex & 0xFFFFFF) | (material.GetShaderType() << 24u);
		for (uint j = groupIndex; j < geometry.meshletCount; j += THREADCOUNT)
		{
			ShaderMeshlet meshlet = (ShaderMeshlet)0;
			meshlet.instanceIndex = instanceIndex;
			meshlet.geometryIndex = geometryIndex;
			meshlet.materialIndex_shaderType = materialIndex_shaderType;

			if (mesh.vb_clu < 0 || mesh.tessellation_factor > 0)
			{
				meshlet.primitiveOffset = j * MESHLET_TRIANGLE_COUNT;
			}
			else
			{
				meshlet.primitiveOffset = (geometry.meshletOffset + j - mesh.meshletOffset) << 7u;
			}

			uint meshletIndex = globalMeshletOffset + geometry.meshletOffset + j;
			output_meshlets[meshletIndex] = meshlet;
		}
	}
}
