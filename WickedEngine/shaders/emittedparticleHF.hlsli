#ifndef WI_EMITTEDPARTICLE_HF
#define WI_EMITTEDPARTICLE_HF
#include "globals.hlsli"
#include "ShaderInterop_EmittedParticle.h"

ShaderMeshInstance EmitterGetInstance()
{
	return load_instance(xEmitterInstanceIndex);
}
ShaderMesh EmitterGetMesh()
{
	return load_mesh(EmitterGetInstance().meshIndex);
}
ShaderMaterial EmitterGetMaterial()
{
	return load_material(load_subset(EmitterGetMesh(), 0).materialIndex);
}

struct VertextoPixel
{
	float4 pos						: SV_POSITION;
	float4 tex						: TEXCOORD0;
	nointerpolation float size		: TEXCOORD1;
	nointerpolation uint color		: TEXCOORD2;
	float3 P : WORLDPOSITION;
	nointerpolation float frameBlend : FRAMEBLEND;
	float2 unrotated_uv : UNROTATED_UV;
};

#endif // WI_EMITTEDPARTICLE_HF
