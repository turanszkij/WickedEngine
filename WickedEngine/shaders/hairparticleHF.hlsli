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
	precise float4 pos : SV_POSITION;
	float clip : SV_ClipDistance0;
	float2 tex : TEXCOORD;
	nointerpolation float fade : DITHERFADE;
	uint primitiveID : PRIMITIVEID;
	half3 nor : NORMAL;
	half wet : WET;
	
	inline float3 GetPos3D()
	{
		return GetCamera().screen_to_world(pos);
	}

	inline float3 GetViewVector()
	{
		return GetCamera().screen_to_nearplane(pos) - GetPos3D(); // ortho support, cannot use cameraPos!
	}
};

#endif // WI_HAIRPARTICLE_HF
