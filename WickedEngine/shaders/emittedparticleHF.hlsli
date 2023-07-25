#ifndef WI_EMITTEDPARTICLE_HF
#define WI_EMITTEDPARTICLE_HF
#include "globals.hlsli"
#include "ShaderInterop_EmittedParticle.h"

ShaderMeshInstance EmitterGetInstance()
{
	return load_instance(xEmitterInstanceIndex);
}
ShaderGeometry EmitterGetGeometry()
{
	return load_geometry(EmitterGetInstance().geometryOffset);
}
ShaderMaterial EmitterGetMaterial()
{
	return load_material(EmitterGetGeometry().materialIndex);
}

struct VertextoPixel
{
	float4 pos : SV_POSITION;
	float4 tex : TEXCOORD0;
	float3 P : WORLDPOSITION;
	float2 unrotated_uv : UNROTATED_UV;
	nointerpolation float frameBlend : FRAMEBLEND;
	nointerpolation float size : PARTICLESIZE;
	nointerpolation uint color : PARTICLECOLOR;
};

#endif // WI_EMITTEDPARTICLE_HF
