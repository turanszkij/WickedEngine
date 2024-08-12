#include "globals.hlsli"

static const uint THREADCOUNT = 64;

RWStructuredBuffer<ShaderMeshlet> output_meshlets : register(u0);

[numthreads(1, THREADCOUNT, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 Gid : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
	uint instanceIndex = Gid.x;
	ShaderMeshInstance inst = load_instance(instanceIndex);

	for (uint i = 0; i < inst.baseGeometryCount; ++i)
	{
		uint geometryIndex = inst.baseGeometryOffset + i;
		ShaderGeometry geometry = load_geometry(geometryIndex);
		if (geometry.vb_msh < 0)
		{
			// Simple meshlet divide:
			for (uint j = groupIndex; j < geometry.meshletCount; j += THREADCOUNT)
			{
				ShaderMeshlet meshlet = (ShaderMeshlet)0;
				meshlet.instanceIndex = instanceIndex;
				meshlet.geometryIndex = geometryIndex;
				meshlet.primitiveOffset = j * MESHLET_TRIANGLE_COUNT;

				uint meshletIndex = inst.meshletOffset + geometry.meshletOffset + j;
				output_meshlets[meshletIndex] = meshlet;
			}
		}
		else
		{
			// Copy and unroll meshlets:
			ByteAddressBuffer vb_meshlets = bindless_buffers[geometry.vb_msh];
			for (uint j = groupIndex; j < geometry.meshletCount; j += THREADCOUNT)
			{
				ShaderMeshlet meshlet = vb_meshlets.Load<ShaderMeshlet>(j * sizeof(ShaderMeshlet));
				meshlet.instanceIndex = instanceIndex;
				meshlet.geometryIndex = geometryIndex;

				uint meshletIndex = inst.meshletOffset + geometry.meshletOffset + j;
				output_meshlets[meshletIndex] = meshlet;
			}
		}
	}
}
