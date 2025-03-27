#include "globals.hlsli"

static const uint THREADCOUNT = 64;

RWStructuredBuffer<ShaderMeshlet> output_meshlets : register(u0);

[numthreads(1, THREADCOUNT, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 Gid : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
	uint instanceIndex = Gid.x;
	ShaderMeshInstance inst = load_instance(instanceIndex);
	ShaderGeometry mesh = load_geometry(inst.baseGeometryOffset);

	for (uint i = 0; i < inst.baseGeometryCount; ++i)
	{
		uint geometryIndex = inst.baseGeometryOffset + i;
		ShaderGeometry geometry = load_geometry(geometryIndex);
		for (uint j = groupIndex; j < geometry.meshletCount; j += THREADCOUNT)
		{
			ShaderMeshlet meshlet = (ShaderMeshlet)0;
			meshlet.instanceIndex = instanceIndex;
			meshlet.geometryIndex = geometryIndex;

			if (mesh.vb_clu < 0)
			{
				meshlet.primitiveOffset = j * MESHLET_TRIANGLE_COUNT;
			}
			else
			{
				meshlet.primitiveOffset = (geometry.meshletOffset + j - mesh.meshletOffset) << 7u;
			}

			uint meshletIndex = inst.meshletOffset + geometry.meshletOffset + j;
			output_meshlets[meshletIndex] = meshlet;
		}
	}
}
