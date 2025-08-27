#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

PUSHCONSTANT(postprocess, PostProcess);

RWTexture2D<unorm float> output : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 Gid : SV_GroupID, uint3 GTid : SV_GroupThreadID)
{
	uint primitiveID = texture_primitiveID[DTid.xy];
	if (primitiveID == 0)
		return;
		
	PrimitiveID prim;
	prim.init();
	prim.unpack(primitiveID);

	ShaderScene scene = GetScene();
	
	if (prim.instanceIndex >= scene.instanceCount)
		return;
	ShaderMeshInstance inst = load_instance(prim.instanceIndex);
	uint geometryIndex = inst.geometryOffset + prim.subsetIndex;
	if (geometryIndex >= scene.geometryCount)
		return;
	ShaderGeometry geometry = load_geometry(geometryIndex);
	if (geometry.vb_pos_wind < 0)
		return;

	if (geometry.materialIndex >= scene.materialCount)
		return;
	ShaderMaterial material = load_material(geometry.materialIndex);
	
	if (material.IsMeshBlend())
	{
		output[DTid.xy] = float(((prim.instanceIndex * geometry.materialIndex) % 255) + 1) / 255.0; // +1: forced request indicator
	}
}
