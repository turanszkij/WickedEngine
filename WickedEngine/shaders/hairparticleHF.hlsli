#ifndef WI_HAIRPARTICLE_HF
#define WI_HAIRPARTICLE_HF
#include "globals.hlsli"
#include "ShaderInterop_HairParticle.h"

ShaderMeshInstance HairGetInstance()
{
	return load_instance(xHairInstanceIndex);
}
ShaderGeometry HairGetGeometry()
{
	return load_geometry(HairGetInstance().geometryOffset);
}
ShaderMaterial HairGetMaterial()
{
	return load_material(HairGetGeometry().materialIndex);
}

struct VertexToPixel
{
	float4 pos : SV_POSITION;
	float2 tex : TEXCOORD;
	nointerpolation float fade : DITHERFADE;
	uint primitiveID : PRIMITIVEID;
	float3 pos3D : POSITION3D;
	float3 nor : NORMAL;
};

#endif // WI_HAIRPARTICLE_HF
